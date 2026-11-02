#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <QSysInfo>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <cstdio>
#include "platform/windows/WindowsGlobalShortcut.h"
#include "platform/windows/WindowsSelectionMonitor.h"
#include "platform/windows/UIAQuery.h"

using namespace ws;
namespace {
void sample(const char* scenario,int cycles) {
    PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);DWORD handles=0;int threads=-1;
    const bool measured=GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
    GetProcessHandleCount(GetCurrentProcess(),&handles);
    const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snapshot!=INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry{};entry.dwSize=sizeof(entry);threads=0;
        if(Thread32First(snapshot,&entry))do {if(entry.th32OwnerProcessID==GetCurrentProcessId())++threads;}while(Thread32Next(snapshot,&entry));
        CloseHandle(snapshot);
    }
    qInfo()<<"M6 Windows resource"<<scenario<<"cycles"<<cycles
        <<"working_set_bytes"<<(measured?qint64(memory.WorkingSetSize):-1)
        <<"private_bytes"<<(measured?qint64(memory.PrivateUsage):-1)<<"handles"<<handles<<"threads"<<threads;
}
void drain() {QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QCoreApplication::processEvents();}
}

class TestWindowsStability final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qInfo()<<"M6 native Windows lifecycle; current-user hooks/hotkeys, synthetic WM_HOTKEY and worker fixtures"
            <<QSysInfo::prettyProductName()<<"Qt"<<qVersion()<<"CPU"<<QSysInfo::currentCpuArchitecture();
    }
    void nativeMonitorRestart100() {
        WindowsSelectionMonitor monitor;
        QVERIFY2(monitor.start(),"Native hook lifecycle unavailable; this is not a desktop application compatibility result.");
        monitor.stop();drain();sample("hooks",0);
        for(int cycle=1;cycle<=100;++cycle) {
            QVERIFY(monitor.start());QVERIFY(monitor.start());QVERIFY(monitor.status().running);
            // Hooks cannot be installed by two monitor owners at once.
            {WindowsSelectionMonitor contender;QVERIFY(!contender.start());}
            QVERIFY(monitor.status().running);monitor.stop();monitor.stop();
            QVERIFY(!monitor.status().running);QVERIFY(!monitor.latestSelection());
            if(cycle%20==0){drain();sample("hooks",cycle);}
        }
        // A new owner must be able to install after the old owner released hooks.
        WindowsSelectionMonitor next;QVERIFY(next.start());next.stop();
        qInfo()<<"M6 native_monitor_restarts=100 plus warmup/probe; no mouse/keyboard injection";
    }
    void nativeHotkeyLifecycle100AndMessages200() {
        WindowsGlobalShortcut shortcut;QSignalSpy activated(&shortcut,&IGlobalShortcut::activated);
        shortcut.start(true);QVERIFY2(shortcut.status().registered,"Ctrl+Alt+P unavailable in this current-user session.");
        shortcut.stop();drain();sample("hotkey",0);
        for(int cycle=1;cycle<=100;++cycle) {
            shortcut.start(true);QVERIFY(shortcut.status().registered);shortcut.start(false);
            const auto hwnd=reinterpret_cast<HWND>(shortcut.nativeWindow());QVERIFY(hwnd);
            for(int event=0;event<2;++event) {
                QVERIFY(PostMessageW(hwnd,WM_HOTKEY,WPARAM(shortcut.nativeId()),MAKELPARAM(MOD_CONTROL|MOD_ALT,'P')));
                QTRY_COMPARE_WITH_TIMEOUT(activated.size(),(cycle-1)*2+event+1,2000);
            }
            shortcut.stop();shortcut.stop();QVERIFY(!shortcut.status().registered);QVERIFY(!IsWindow(hwnd));
            // The production object must not retain the global key combination.
            constexpr int probe=0x6421;
            QVERIFY(RegisterHotKey(nullptr,probe,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'P'));
            QVERIFY(UnregisterHotKey(nullptr,probe));
            drain();QCOMPARE(activated.size(),cycle*2);
            if(cycle%20==0)sample("hotkey",cycle);
        }
        qInfo()<<"M6 native_shortcut_register_unregister=100 synthetic_WM_HOTKEY_activations=200; physical keyboard acceptance NOT TESTED";
    }
    void workerTimeoutCancelRecoveryAndShutdown() {
        // This executable's numeric argument mode is a synthetic helper using
        // the exact production UIAQuery subprocess protocol, not real app UIA.
        UIAQuery query(nullptr,QCoreApplication::applicationFilePath(),1000);
        QSignalSpy done(&query,&UIAQuery::completed);
        QTimer heartbeat;heartbeat.setInterval(10);int beats=0;
        connect(&heartbeat,&QTimer::timeout,this,[&]{++beats;});heartbeat.start();
        QVERIFY(query.request(2));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,3000);done.clear();
        QTRY_VERIFY(query.findChildren<QProcess*>().isEmpty());drain();sample("synthetic_uia_worker",0);
        for(int cycle=1;cycle<=10;++cycle) {
            const int before=beats;
            QVERIFY(query.request(1));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,3000);
            const auto timeout=qvariant_cast<UIAQueryResult>(done.takeFirst()[0]);
            QCOMPARE(timeout.reason,QString("UIA query timed out"));QVERIFY(!timeout.selection);QVERIFY(!query.busy());QVERIFY(beats>before);
            QTRY_VERIFY_WITH_TIMEOUT(query.findChildren<QProcess*>().isEmpty(),3000);
            QVERIFY(query.request(1));QTRY_VERIFY_WITH_TIMEOUT(([&] {
                const auto children=query.findChildren<QProcess*>();return !children.isEmpty() && children.first()->state()==QProcess::Running;
            })(),2000);
            query.cancel();QVERIFY(!query.busy());QCOMPARE(done.size(),0);
            QTRY_VERIFY_WITH_TIMEOUT(query.findChildren<QProcess*>().isEmpty(),3000);
            QVERIFY(query.request(2));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,3000);
            const auto recovered=qvariant_cast<UIAQueryResult>(done.takeFirst()[0]);
            QVERIFY(recovered.initialized);QVERIFY(recovered.selection);QVERIFY(!query.busy());
            QTRY_VERIFY_WITH_TIMEOUT(query.findChildren<QProcess*>().isEmpty(),3000);
            drain();QCOMPARE(done.size(),0);if(cycle%2==0)sample("synthetic_uia_worker",cycle);
        }
        HANDLE childHandle=nullptr;
        {
            auto pending=std::make_unique<UIAQuery>(nullptr,QCoreApplication::applicationFilePath(),30000);
            QVERIFY(pending->request(1));
            QTRY_VERIFY_WITH_TIMEOUT(([&] {
                const auto children=pending->findChildren<QProcess*>();
                return !children.isEmpty() && children.first()->state()==QProcess::Running;
            })(),3000);
            const auto pid=pending->findChildren<QProcess*>().first()->processId();
            childHandle=OpenProcess(SYNCHRONIZE,FALSE,DWORD(pid));QVERIFY(childHandle);
            QElapsedTimer shutdown;shutdown.start();pending.reset();QVERIFY(shutdown.elapsed()<2000);
        }
        struct CloseChild {HANDLE handle;~CloseChild(){if(handle)CloseHandle(handle);}} closeChild{childHandle};
        QTRY_COMPARE_WITH_TIMEOUT(WaitForSingleObject(childHandle,0),DWORD(WAIT_OBJECT_0),3000);
        qInfo()<<"M6 synthetic_UIA timeout=10 cancellation=10 recovery=10 pending_shutdown=1; timeout heartbeat responsive; helpers reaped; native provider hang acceptance NOT TESTED";
    }
};

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(argc==3) {
        bool numeric=false;const auto mode=app.arguments().value(1).toInt(&numeric);
        if(numeric && mode==1){QTimer::singleShot(30000,&app,&QCoreApplication::quit);return app.exec();}
        if(numeric && mode==2) {
            const QByteArray payload=R"({"initialized":true,"reason":"selection","selection":{"text":"Synthetic UIA selection.","source":"fixture","anchor":[10,20,80,20]}})";
            std::fwrite(payload.constData(),1,size_t(payload.size()),stdout);return 0;
        }
    }
    TestWindowsStability test;return QTest::qExec(&test,argc,argv);
}
#include "TestWindowsStability.moc"
