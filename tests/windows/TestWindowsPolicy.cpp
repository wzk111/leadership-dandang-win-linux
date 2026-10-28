#include <QtTest>
#include <QPushButton>
#include <QScreen>
#include <QProcess>
#include <QFile>
#include "platform/windows/WindowsWindowPolicy.h"
#include "platform/windows/UIAQuery.h"
#include "ui/ActionBar.h"
#include <windows.h>
#include "WindowsFixture.h"
using namespace ws;
class TestWindowsPolicy : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        // QTEST_MAIN has initialized QApplication; no test QWidget exists yet.
        if(QGuiApplication::platformName()!="windows")QSKIP("DPI awareness verification requires native Qt Windows platform.");
        const auto context=GetThreadDpiAwarenessContext();
        QVERIFY2(context,"Windows did not return a thread DPI awareness context.");
        const auto awareness=GetAwarenessFromDpiAwarenessContext(context);
        const bool perMonitorV2=AreDpiAwarenessContextsEqual(context,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        qInfo("Qt startup DPI awareness: %d; per-monitor V2: %s",int(awareness),perMonitorV2?"yes":"no");
        QCOMPARE(int(awareness),int(DPI_AWARENESS_PER_MONITOR_AWARE));
    }
    void scales_data() {
        QTest::addColumn<double>("scale");
        for(double scale:{1.0,1.25,1.5,2.0})QTest::newRow(qPrintable(QString::number(scale)))<<scale;
    }
    void scales() {
        QFETCH(double,scale);
        const QRect physical(-2400,-1200,2400,1200),logical(-1920,-960,1920,960);
        const QRect anchor(-2400+int(200*scale),-1200+int(100*scale),int(80*scale),int(20*scale));
        const auto mapped=WindowsWindowPolicy::physicalToLogical(anchor,physical,logical,scale);
        QVERIFY(mapped);QCOMPARE(*mapped,QRect(-1720,-860,80,20));
    }
    void rejectsInvalidGeometry() {
        QVERIFY(!WindowsWindowPolicy::physicalToLogical({},QRect(0,0,100,100),QRect(0,0,100,100),1));
        QVERIFY(!WindowsWindowPolicy::physicalToLogical(QRect(1,1,5,5),QRect(0,0,100,100),QRect(0,0,100,100),0));
    }
    void nativeMonitorIdentityAndPlacement() {
        WindowsWindowPolicy policy;ActionBar bar;QVERIFY(policy.configureActionBar(bar));
        for(auto* screen:QGuiApplication::screens()) {
            const auto* native=screen->nativeInterface<QNativeInterface::QWindowsScreen>();QVERIFY(native);
            MONITORINFOEXW info{};info.cbSize=sizeof(info);
            QVERIFY(GetMonitorInfoW(native->handle(),reinterpret_cast<MONITORINFO*>(&info)));
            const QRect anchor(info.rcMonitor.left+100,info.rcMonitor.top+100,100,20);
            const auto placement=policy.positionActionBar(bar,anchor);QVERIFY(placement);
            QVERIFY(screen->availableGeometry().contains(placement->geometry));
        }
    }
    void nativeStylesAndMouseActivation() {
        if(QGuiApplication::platformName()!="windows")QSKIP("Requires native Qt Windows platform.");
        WindowsWindowPolicy policy;ActionBar bar;
        QVERIFY(policy.configureActionBar(bar));
        const auto hwnd=reinterpret_cast<HWND>(bar.winId());
        const auto style=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
        QVERIFY(style&WS_EX_NOACTIVATE);QVERIFY(style&WS_EX_TOOLWINDOW);
        QVERIFY(!(style&WS_EX_TRANSPARENT));QVERIFY(!(style&WS_EX_LAYERED));
        MSG message{};message.hwnd=hwnd;message.message=WM_MOUSEACTIVATE;qintptr result=0;
        QVERIFY(policy.nativeEventFilter("windows_generic_MSG",&message,&result));QCOMPARE(result,qintptr(MA_NOACTIVATE));
    }
    void snapshotClick() {
        WindowsWindowPolicy policy;ActionBar bar;QVERIFY(policy.configureActionBar(bar));
        QSignalSpy chosen(&bar,&ActionBar::featureChosen);
        bar.showForSelection({"selection A",QRect(20,20,80,20),"fixture"},QPoint(30,50));
        auto* button=bar.findChild<QPushButton*>("action2");QVERIFY(button);
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(chosen.size(),1);
        QVERIFY(qvariant_cast<Selection>(chosen[0][1]).text=="selection A");
    }
    void nativeForegroundPreserved() {
        if(qEnvironmentVariable("WORKSIDEKICK_WINDOWS_INTERACTIVE")!="1")QSKIP("Explicit interactive desktop opt-in required for foreground test.");
        if(QGuiApplication::platformName()!="windows")QSKIP("Requires native Windows platform.");
        const auto desktop=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
        if(!desktop)QSKIP("Interactive input desktop unavailable; native focus NOT TESTED.");
        CloseDesktop(desktop);
        const auto fixturePath=QCoreApplication::applicationDirPath()+"/test_uia.exe";
        const auto workerPath=QCoreApplication::applicationDirPath()+"/worksidekick_uia_worker.exe";
        QVERIFY2(QFile::exists(fixturePath),"Build test_uia beside test_windowspolicy for the external synthetic fixture.");
        QVERIFY2(QFile::exists(workerPath),"Build worksidekick_uia_worker beside the policy test.");
        QProcess fixture;
        struct FixtureCleanup {
            QProcess& process;HWND window=nullptr;
            ~FixtureCleanup() {
                if(window && IsWindow(window))PostMessageW(window,WM_CLOSE,0,0);
                if(process.state()!=QProcess::NotRunning && !process.waitForFinished(1000)){process.kill();process.waitForFinished(1000);}
            }
        } cleanup{fixture};
        fixture.start(fixturePath,{"--uia-fixture"});QVERIFY(fixture.waitForStarted(3000));
        if(!fixture.bytesAvailable())QVERIFY(fixture.waitForReadyRead(5000));
        bool valid=false;const auto handle=fixture.readLine().trimmed().toULongLong(&valid);QVERIFY(valid);
        const auto source=reinterpret_cast<HWND>(quintptr(handle));DWORD sourcePid=0;
        QVERIFY(IsWindow(source));QVERIFY(GetWindowThreadProcessId(source,&sourcePid));
        QCOMPARE(sourcePid,DWORD(fixture.processId()));cleanup.window=source;
        AllowSetForegroundWindow(DWORD(fixture.processId()));
        DWORD_PTR ignored=0;
        QVERIFY(SendMessageTimeoutW(source,WM_APP+1,0,0,SMTO_ABORTIFHUNG,2000,&ignored));
        QString activationReason;
        if(!ws::test::prepareFixtureForeground(source,sourcePid,activationReason))QSKIP(qPrintable(activationReason));

        WindowsWindowPolicy policy;ActionBar bar;QVERIFY(policy.configureActionBar(bar));
        QSignalSpy chosen(&bar,&ActionBar::featureChosen);
        UIAQuery query(nullptr,workerPath,5000);QSignalSpy results(&query,&UIAQuery::completed);
        Selection snapshot;
        for(int stage=0;stage<4;++stage) {
            if(stage==1 || stage==3) {
                QVERIFY(snapshot.anchorRect);
                const auto placement=policy.positionActionBar(bar,*snapshot.anchorRect);
                if(!placement) {
                    const auto anchor=*snapshot.anchorRect;
                    qInfo()<<"Placement failed at stage"<<stage<<"physical anchor"<<anchor
                           <<"bar size"<<bar.size()<<"Qt platform"<<QGuiApplication::platformName()
                           <<"last Win32 error"<<GetLastError();
                    RECT rectangle{anchor.x(),anchor.y(),LONG(qint64(anchor.x())+anchor.width()),LONG(qint64(anchor.y())+anchor.height())};
                    const auto monitor=MonitorFromRect(&rectangle,MONITOR_DEFAULTTONEAREST);
                    MONITORINFOEXW info{};info.cbSize=sizeof(info);
                    const bool monitorInfo=monitor && GetMonitorInfoW(monitor,reinterpret_cast<MONITORINFO*>(&info));
                    qInfo()<<"MonitorFromRect handle"<<quintptr(monitor)<<"GetMonitorInfo success"<<monitorInfo
                           <<"last Win32 error"<<GetLastError();
                    const auto device=monitorInfo?QString::fromWCharArray(info.szDevice):QString();
                    const QRect physical(info.rcMonitor.left,info.rcMonitor.top,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top);
                    qInfo()<<"Native monitor device"<<device<<"physical geometry"<<physical;
                    for(auto* screen:QGuiApplication::screens()) {
                        const auto* native=screen->nativeInterface<QNativeInterface::QWindowsScreen>();
                        const bool matched=monitorInfo && native && native->handle()==monitor;
                        qInfo()<<"Qt screen"<<screen->name()<<"geometry"<<screen->geometry()
                               <<"available"<<screen->availableGeometry()<<"DPR"<<screen->devicePixelRatio()<<"device matched"<<matched;
                        if(matched) {
                            const auto mapped=WindowsWindowPolicy::physicalToLogical(anchor,physical,screen->geometry(),screen->devicePixelRatio());
                            qInfo()<<"Mapped anchor valid"<<bool(mapped)<<"logical anchor"<<mapped.value_or(QRect());
                            if(mapped)qInfo()<<"Pure placement available"<<bool(placeActionBar(*mapped,bar.size(),screen->availableGeometry()));
                        }
                    }
                }
                QVERIFY2(placement,"Native toolbar placement failed; see physical monitor / Qt screen diagnostics above.");
                bar.showForSelection(snapshot,placement->geometry.topLeft());QTest::qWait(100);
                QCOMPARE(GetForegroundWindow(),source);
            }
            if(stage==2) {bar.hideBar();QTest::qWait(50);QCOMPARE(GetForegroundWindow(),source);}
            if(stage==3) {
                auto* button=bar.findChild<QPushButton*>("action2");QVERIFY(button);QVERIFY(button->isVisible());
                RECT rectangle{};QVERIFY(GetWindowRect(reinterpret_cast<HWND>(button->winId()),&rectangle));
                const POINT point{rectangle.left+(rectangle.right-rectangle.left)/2,rectangle.top+(rectangle.bottom-rectangle.top)/2};
                // Test-only native input is confined to this test's visible toolbar.
                QVERIFY(GetAncestor(WindowFromPoint(point),GA_ROOT)==reinterpret_cast<HWND>(bar.winId()));
                POINT previous{};QVERIFY(GetCursorPos(&previous));
                struct RestoreCursor {POINT point;~RestoreCursor(){SetCursorPos(point.x,point.y);}} restore{previous};
                QVERIFY(SetCursorPos(point.x,point.y));
                INPUT input[2]{};input[0].type=INPUT_MOUSE;input[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
                input[1].type=INPUT_MOUSE;input[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
                QCOMPARE(SendInput(2,input,sizeof(INPUT)),UINT(2));
                QTRY_COMPARE(chosen.size(),1);
                QVERIFY(qvariant_cast<Selection>(chosen[0][1]).text==snapshot.text);
                QVERIFY(!bar.isVisible());QCOMPARE(GetForegroundWindow(),source);
            }
            QVERIFY(query.request(quintptr(handle)));QTRY_COMPARE_WITH_TIMEOUT(results.size(),1,6000);
            const auto result=qvariant_cast<UIAQueryResult>(results.takeFirst()[0]);
            QVERIFY2(result.initialized,qPrintable(result.reason));QVERIFY2(result.selection.has_value(),qPrintable(result.reason));
            QCOMPARE(result.selection->text,QString("  synthetic selection  "));
            QCOMPARE(GetForegroundWindow(),source);
            if(stage==0)snapshot=*result.selection;
        }
    }
};
QTEST_MAIN(TestWindowsPolicy)
#include "TestWindowsPolicy.moc"
