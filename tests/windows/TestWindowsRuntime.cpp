#include <QtTest>
#include <QGuiApplication>
#include <QProcess>
#include <QFile>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <array>
#include "platform/windows/WindowsSelectionMonitor.h"
#include "platform/windows/WindowsGlobalShortcut.h"
#include "platform/windows/UIAQuery.h"
#include <windows.h>
#include "WindowsFixture.h"
using namespace ws;
namespace {
struct InjectionResult {UINT sent; qint64 elapsedMs;};
template<size_t N> auto injectNativeInput(std::array<INPUT,N> input){
    // WH_MOUSE_LL is serviced on the Qt thread. Keep that message pump running
    // while SendInput waits for hook delivery; the worker owns its INPUT copy.
    return QtConcurrent::run([input]()mutable{
        QElapsedTimer timer;timer.start();const auto sent=SendInput(UINT(N),input.data(),sizeof(INPUT));
        return InjectionResult{sent,timer.elapsed()};
    });
}
class RecordingMonitor final:public WindowsSelectionMonitor {
public:
    explicit RecordingMonitor(quintptr fixture):fixture_(fixture){clock_.start();}
    void dumpTrace()const {
        qInfo()<<"Native gesture thresholds: doubleClickMs="<<GetDoubleClickTime()
            <<"doubleClickHalfWidth="<<GetSystemMetrics(SM_CXDOUBLECLK)/2
            <<"doubleClickHalfHeight="<<GetSystemMetrics(SM_CYDOUBLECLK)/2
            <<"dragWidth="<<GetSystemMetrics(SM_CXDRAG)<<"dragHeight="<<GetSystemMetrics(SM_CYDRAG);
        for(const auto& line:trace_)qInfo().noquote()<<line;
        if(dropped_)qInfo()<<"Native trace entries dropped after bounded limit="<<dropped_;
    }
protected:
    void processMouse(bool down,QPoint point,quint32 timestamp,quintptr target)override {
        const auto event=++event_;
        if(target==fixture_)record(QString("event=%1 elapsed=%2 mouse=%3 x=%4 y=%5 timestamp=%6 fixture=%7")
            .arg(event).arg(clock_.elapsed()).arg(down?"down":"up").arg(point.x()).arg(point.y()).arg(timestamp).arg(qulonglong(fixture_)));
        else record(QString("event=%1 elapsed=%2 mouse outside fixture [coordinates, timestamp and identity redacted]").arg(event).arg(clock_.elapsed()));
        WindowsSelectionMonitor::processMouse(down,point,timestamp,target);
        record(QString("event=%1 after mouse: acceptedGestures=%2 result=%3").arg(event).arg(status().events).arg(status().lastResult));
    }
    void processForeground(quintptr window)override {
        const auto event=++event_;
        record(QString("event=%1 elapsed=%2 foreground=%3").arg(event).arg(clock_.elapsed())
            .arg(window==fixture_?QString("fixture %1").arg(qulonglong(fixture_)):QString("outside fixture [identity redacted]")));
        WindowsSelectionMonitor::processForeground(window);
        record(QString("event=%1 after foreground: acceptedGestures=%2 result=%3").arg(event).arg(status().events).arg(status().lastResult));
    }
private:
    void record(const QString& line){if(trace_.size()<128)trace_<<line;else ++dropped_;}
    quintptr fixture_;QElapsedTimer clock_;QStringList trace_;quint64 event_=0,dropped_=0;
};
struct FixtureCleanup {
    QProcess& process;HWND window=nullptr;
    ~FixtureCleanup(){
        if(window && IsWindow(window))PostMessageW(window,WM_CLOSE,0,0);
        if(process.state()!=QProcess::NotRunning && !process.waitForFinished(1000)){process.kill();process.waitForFinished(1000);}
    }
};
struct InputCleanup {
    POINT cursor;bool mousePending=false,keysPending=false;
    ~InputCleanup(){
        // Only release inputs introduced by a partially successful test injection.
        if(mousePending){INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&input,sizeof(INPUT));}
        if(keysPending){INPUT input[3]{};const WORD keys[]{'P',VK_MENU,VK_CONTROL};
            for(int i=0;i<3;++i){input[i].type=INPUT_KEYBOARD;input[i].ki.wVk=keys[i];input[i].ki.dwFlags=KEYEVENTF_KEYUP;}
            SendInput(3,input,sizeof(INPUT));
        }
        SetCursorPos(cursor.x,cursor.y);
    }
};
}
class TestWindowsRuntime:public QObject {
 Q_OBJECT
private slots:
 void syntheticMouseHookAndGlobalHotkey(){
    // Native acceptance test, not a mock: no AI provider, network service, app controller,
    // clipboard reader or user-window traversal is constructed anywhere in this test.
    if(qEnvironmentVariable("WORKSIDEKICK_WINDOWS_INTERACTIVE")!="1")QSKIP("Native runtime NOT TESTED: explicit interactive opt-in required.");
    if(QGuiApplication::platformName()!="windows")QSKIP("Native runtime NOT TESTED: native Qt Windows platform required.");
    const auto desktop=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
    if(!desktop)QSKIP("Native runtime NOT TESTED: interactive input desktop unavailable.");
    CloseDesktop(desktop);
    for(const int key:{VK_CONTROL,VK_MENU,VK_SHIFT,VK_LWIN,VK_RWIN,VK_LBUTTON,VK_RBUTTON})
        if(GetAsyncKeyState(key)&0x8000)QSKIP("Native runtime NOT TESTED: release held modifiers and mouse buttons first.");
    const auto fixturePath=QCoreApplication::applicationDirPath()+"/test_uia.exe";
    const auto workerPath=QCoreApplication::applicationDirPath()+"/worksidekick_uia_worker.exe";
    QVERIFY2(QFile::exists(fixturePath),"Build test_uia beside test_windowsruntime.");
    QVERIFY2(QFile::exists(workerPath),"Build worksidekick_uia_worker beside test_windowsruntime.");
    QProcess fixture;FixtureCleanup cleanup{fixture};
    fixture.start(fixturePath,{"--uia-fixture"});QVERIFY(fixture.waitForStarted(3000));
    if(!fixture.bytesAvailable())QVERIFY(fixture.waitForReadyRead(5000));
    bool valid=false;const auto handle=fixture.readLine().trimmed().toULongLong(&valid);QVERIFY(valid);
    const auto source=reinterpret_cast<HWND>(quintptr(handle));DWORD sourcePid=0;
    QVERIFY(IsWindow(source));QVERIFY(GetWindowThreadProcessId(source,&sourcePid));
    QCOMPARE(sourcePid,DWORD(fixture.processId()));cleanup.window=source;
    AllowSetForegroundWindow(sourcePid);DWORD_PTR ignored=0;
    QVERIFY(SendMessageTimeoutW(source,WM_APP+1,0,0,SMTO_ABORTIFHUNG,2000,&ignored));
    QString activationReason;
    if(!ws::test::prepareFixtureForeground(source,sourcePid,activationReason))QSKIP(qPrintable(activationReason));
    POINT previous{};QVERIFY(GetCursorPos(&previous));InputCleanup inputs{previous};

    UIAQuery query(nullptr,workerPath,5000);QSignalSpy queried(&query,&UIAQuery::completed);
    QVERIFY(query.request(quintptr(handle)));QTRY_COMPARE_WITH_TIMEOUT(queried.size(),1,6000);
    const auto initial=qvariant_cast<UIAQueryResult>(queried.takeFirst()[0]);
    QVERIFY2(initial.initialized,qPrintable(initial.reason));QVERIFY2(initial.selection.has_value(),qPrintable(initial.reason));
    QCOMPARE(initial.selection->text,QString("  synthetic selection  "));QVERIFY(initial.selection->anchorRect);
    const auto rect=*initial.selection->anchorRect;QVERIFY(rect.isValid());
    // The first quarter of this fixed one-line synthetic phrase lies inside "synthetic".
    const POINT point{rect.left()+rect.width()/4,rect.top()+rect.height()/2};
    QCOMPARE(GetForegroundWindow(),source);QCOMPARE(GetAncestor(WindowFromPoint(point),GA_ROOT),source);
    RecordingMonitor monitor{quintptr(handle)};QSignalSpy selected(&monitor,&ISelectionMonitor::selectionDetected);
    QVERIFY2(monitor.start(),qPrintable(monitor.status().description));QVERIFY(monitor.status().running);
    QCOMPARE(selected.size(),0);QVERIFY(SetCursorPos(point.x,point.y));
    QCOMPARE(GetForegroundWindow(),source);QCOMPARE(GetAncestor(WindowFromPoint(point),GA_ROOT),source);
    std::array<INPUT,4> mouse{};
    for(int i=0;i<4;++i){mouse[i].type=INPUT_MOUSE;mouse[i].mi.dwFlags=i%2?MOUSEEVENTF_LEFTUP:MOUSEEVENTF_LEFTDOWN;}
    inputs.mousePending=true;auto mouseInjection=injectNativeInput(mouse);
    QElapsedTimer mouseWait;mouseWait.start();
    while(!mouseInjection.isFinished() && mouseWait.elapsed()<5000)QTest::qWait(10);
    qInfo()<<"Mouse injection worker: finished="<<mouseInjection.isFinished()<<"pumpWaitMs="<<mouseWait.elapsed();
    QVERIFY2(mouseInjection.isFinished(),"Native mouse injection exceeded 5000 ms while the Qt thread continued pumping messages.");
    const auto mouseResult=mouseInjection.result();if(mouseResult.sent==4)inputs.mousePending=false;
    qInfo()<<"Mouse injection sent="<<mouseResult.sent<<"injectElapsedMs="<<mouseResult.elapsedMs;
    QCOMPARE(mouseResult.sent,UINT(4));
    QElapsedTimer detectionWait;detectionWait.start();
    while(selected.isEmpty() && detectionWait.elapsed()<6000)QTest::qWait(20);
    const auto detectedWithinDeadline=selected.size();
    const auto monitorState=monitor.status();const auto latest=monitor.latestSelection();
    qInfo()<<"Native monitor diagnostic: elapsedMs="<<detectionWait.elapsed()<<"detected="<<selected.size()
        <<"events="<<monitorState.events<<"initialized="<<monitorState.initialized
        <<"running="<<monitorState.running<<"lastResult="<<monitorState.lastResult<<"description="<<monitorState.description
        <<"foreground="<<reinterpret_cast<quintptr>(GetForegroundWindow())<<"fixture="<<quintptr(handle)
        <<"latestChars="<<(latest?latest->text.size():0)<<"latestAnchor="<<bool(latest && latest->anchorRect);
    if(selected.isEmpty()){
        monitor.dumpTrace();
        // Probe only the same synthetic fixture, separating native selection from hook delivery.
        QVERIFY(query.request(quintptr(handle)));QElapsedTimer probeWait;probeWait.start();
        while(queried.isEmpty() && probeWait.elapsed()<6000)QTest::qWait(20);
        if(!queried.isEmpty()){
            const auto probe=qvariant_cast<UIAQueryResult>(queried.takeFirst()[0]);
            const bool knownSynthetic=probe.selection && !probe.selection->text.isEmpty() && initial.selection->text.contains(probe.selection->text);
            qInfo()<<"Post-doubleclick UIA: initialized="<<probe.initialized<<"reason="<<probe.reason
                <<"chars="<<(probe.selection?probe.selection->text.size():0)<<"anchor="<<bool(probe.selection && probe.selection->anchorRect)
                <<"isSyntheticWord="<<bool(probe.selection && probe.selection->text.trimmed()=="synthetic")
                <<"fixtureText="<<(knownSynthetic?probe.selection->text:QString("[not fixture text or unavailable]"));
        }else qInfo()<<"Post-doubleclick UIA: no completion within 6000 ms";
    }
    QCOMPARE(detectedWithinDeadline,1);
    const auto snapshot=qvariant_cast<Selection>(selected[0][0]);
    QCOMPARE(snapshot.text.trimmed(),QString("synthetic"));QVERIFY(initial.selection->text.contains(snapshot.text));
    QVERIFY(snapshot.anchorRect);QVERIFY(snapshot.anchorRect->isValid());QVERIFY(!snapshot.sourceApplication.isEmpty());
    QVERIFY(monitor.status().initialized);QVERIFY(monitor.status().events>=1);
    QVERIFY(monitor.latestSelection());QCOMPARE(monitor.latestSelection()->text,snapshot.text);QCOMPARE(GetForegroundWindow(),source);

    WindowsGlobalShortcut shortcut;QSignalSpy activated(&shortcut,&IGlobalShortcut::activated);
    shortcut.start(true);QVERIFY2(shortcut.status().registered,qPrintable(shortcut.status().description));
    std::array<INPUT,6> keyboard{};const WORD keys[]{VK_CONTROL,VK_MENU,'P','P',VK_MENU,VK_CONTROL};
    for(int i=0;i<6;++i){keyboard[i].type=INPUT_KEYBOARD;keyboard[i].ki.wVk=keys[i];if(i>=3)keyboard[i].ki.dwFlags=KEYEVENTF_KEYUP;}
    QCOMPARE(GetForegroundWindow(),source);inputs.keysPending=true;
    auto keyInjection=injectNativeInput(keyboard);QElapsedTimer keyWait;keyWait.start();
    while(!keyInjection.isFinished() && keyWait.elapsed()<5000)QTest::qWait(10);
    qInfo()<<"Key injection worker: finished="<<keyInjection.isFinished()<<"pumpWaitMs="<<keyWait.elapsed();
    QVERIFY2(keyInjection.isFinished(),"Native key injection exceeded 5000 ms while the Qt thread continued pumping messages.");
    const auto keyResult=keyInjection.result();if(keyResult.sent==6)inputs.keysPending=false;
    qInfo()<<"Key injection sent="<<keyResult.sent<<"injectElapsedMs="<<keyResult.elapsedMs;
    QCOMPARE(keyResult.sent,UINT(6));QTRY_COMPARE_WITH_TIMEOUT(activated.size(),1,3000);
    QVERIFY(shortcut.status().lastActivation.isValid());QCOMPARE(GetForegroundWindow(),source);
    QVERIFY(query.request(quintptr(handle)));QTRY_COMPARE_WITH_TIMEOUT(queried.size(),1,6000);
    const auto after=qvariant_cast<UIAQueryResult>(queried.takeFirst()[0]);
    QVERIFY2(after.initialized,qPrintable(after.reason));QVERIFY2(after.selection.has_value(),qPrintable(after.reason));
    QCOMPARE(after.selection->text,snapshot.text);QCOMPARE(after.selection->anchorRect,snapshot.anchorRect);
    QCOMPARE(selected.size(),1);QCOMPARE(activated.size(),1);QVERIFY(monitor.latestSelection());
    QCOMPARE(monitor.latestSelection()->text,snapshot.text);
    shortcut.stop();QVERIFY(!shortcut.status().registered);monitor.stop();QVERIFY(!monitor.status().running);
 }
};
QTEST_MAIN(TestWindowsRuntime)
#include "TestWindowsRuntime.moc"
