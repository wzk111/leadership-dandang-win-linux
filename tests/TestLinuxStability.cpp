#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSysInfo>
#include "platform/linux/AtSpiSelectionMonitor.h"
#include "platform/linux/X11GlobalShortcut.h"
#include <X11/Xlib.h>
#undef None

using namespace ws;
namespace {
void sample(const char* scenario,int completed) {
    qint64 rss=-1,threads=-1;QFile status("/proc/self/status");
    if(status.open(QIODevice::ReadOnly)) {
        const auto data=QString::fromLatin1(status.readAll());
        auto match=QRegularExpression("VmRSS:\\s+(\\d+)").match(data);
        if(match.hasMatch())rss=match.captured(1).toLongLong()*1024;
        match=QRegularExpression("Threads:\\s+(\\d+)").match(data);
        if(match.hasMatch())threads=match.captured(1).toLongLong();
    }
    qInfo()<<"M6 Linux resource"<<scenario<<"completed"<<completed<<"rss_bytes"<<rss<<"threads"<<threads
        <<"fds"<<QDir("/proc/self/fd").entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size();
}
void drain(){QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QCoreApplication::processEvents();}
class SimulatedSession final : public IAtSpiSession {
public:
    int starts=0,stops=0,reads=0;bool pending=false;quint64 lastSequence=0;
    void start() override {
        ++starts;MonitorStatus status;status.available=status.initialized=status.running=true;
        status.description="Simulated AT-SPI session";emit state(status);
    }
    void stop() override {++stops;}
    void extract(quint64 sequence) override {++reads;lastSequence=sequence;pending=true;}
    void complete(quint64 sequence) {
        pending=false;emit extracted(sequence,{Selection{QString("Synthetic event %1.").arg(sequence),QRect(20,30,100,20),"fixture"},"synthetic result"});
    }
};
}

class TestLinuxStability final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qInfo()<<"M6 Linux stability"<<QSysInfo::prettyProductName()<<"Qt"<<qVersion()
            <<"backend"<<QGuiApplication::platformName()<<"CPU"<<QSysInfo::currentCpuArchitecture()
            <<"session"<<qEnvironmentVariable("XDG_SESSION_TYPE","unavailable")
            <<"desktop"<<qEnvironmentVariable("XDG_CURRENT_DESKTOP","unavailable")
            <<"Synthetic transport and Xvfb are Level A evidence, not GNOME desktop acceptance.";
    }
    void simulatedEventsDebouncePendingStopAndRestart() {
        auto session=std::make_unique<SimulatedSession>();auto* fixture=session.get();
        AtSpiSelectionMonitor monitor(std::move(session));QSignalSpy selected(&monitor,&ISelectionMonitor::selectionDetected);
        QVERIFY(monitor.start());emit fixture->eventReceived(1);QTRY_COMPARE(fixture->reads,1);fixture->complete(1);
        monitor.stop();selected.clear();drain();sample("simulated_atspi",0);
        quint64 sequence=1;const int starts=fixture->starts,stops=fixture->stops,reads=fixture->reads;
        for(int cycle=1;cycle<=100;++cycle) {
            QVERIFY(monitor.start());QVERIFY(monitor.start());QVERIFY(monitor.status().running);
            // Ten events arrive in one burst. The production 80ms debounce
            // intentionally coalesces them into one extraction; never call this
            // 1000 independent native extractions.
            for(int event=0;event<10;++event)emit fixture->eventReceived(++sequence);
            QTRY_COMPARE_WITH_TIMEOUT(fixture->reads,reads+cycle,1000);QVERIFY(fixture->pending);
            QCOMPARE(fixture->lastSequence,sequence);
            // An earlier extraction is ignored, even while enabled.
            fixture->complete(sequence-1);QVERIFY(!monitor.latestSelection());
            fixture->complete(sequence);QVERIFY(monitor.latestSelection());QCOMPARE(selected.size(),cycle);
            // Duplicate result must not produce a second selection delivery.
            fixture->complete(sequence);QCOMPARE(selected.size(),cycle);
            monitor.stop();QVERIFY(!monitor.latestSelection());QVERIFY(!monitor.status().running);
            fixture->complete(sequence);QVERIFY(!monitor.latestSelection());QCOMPARE(selected.size(),cycle);
            if(cycle%20==0){drain();sample("simulated_atspi",cycle);}
        }
        QCOMPARE(fixture->starts-starts,100);QCOMPARE(fixture->stops-stops,100);
        QCOMPARE(monitor.status().events,quint64(1001));
        // Explicitly stop while a production debounce is queued. No extraction
        // may run after stop, and a late reply must remain ignored.
        QVERIFY(monitor.start());emit fixture->eventReceived(++sequence);monitor.stop();
        QTimer boundary;boundary.setSingleShot(true);boundary.start(120);
        QTRY_VERIFY_WITH_TIMEOUT(!boundary.isActive(),1000);QCOMPARE(fixture->reads,reads+100);
        fixture->complete(sequence);QVERIFY(!monitor.latestSelection());
        qInfo()<<"M6 simulated_ATSPI input_events=1000 coalesced_extractions=100 delivered_selections=100 monitor_restarts=100 stale_results_ignored=200 pending_stop=1; native AT-SPI recovery NOT implied";
    }
    void nativeX11GrabLifecycle100() {
        if(QGuiApplication::platformName()!="xcb")QSKIP("NOT TESTED — X11 Qt backend unavailable; no GNOME result inferred.");
        X11GlobalShortcut shortcut;QSignalSpy activation(&shortcut,&IGlobalShortcut::activated);
        shortcut.start(true);QTRY_VERIFY_WITH_TIMEOUT(shortcut.status().registered,5000);shortcut.stop();drain();sample("native_x11_grab",0);
        for(int cycle=1;cycle<=100;++cycle) {
            shortcut.start(true);QTRY_VERIFY_WITH_TIMEOUT(shortcut.status().registered,3000);
            if(cycle%25==0) {
                X11GlobalShortcut collision;collision.start(true);
                QTRY_VERIFY_WITH_TIMEOUT(collision.status().description.contains("failed"),3000);
                QVERIFY(!collision.status().registered);collision.stop();QVERIFY(shortcut.status().registered);
            }
            shortcut.stop();shortcut.stop();QVERIFY(!shortcut.status().registered);
            drain();QCOMPARE(activation.size(),0);
            if(cycle%20==0)sample("native_x11_grab",cycle);
        }
        X11GlobalShortcut next;next.start(true);QTRY_VERIFY_WITH_TIMEOUT(next.status().registered,3000);next.stop();
        qInfo()<<"M6 native_X11_register_unregister=100 collision_probes=4; no input injection; real GNOME/Wayland portal acceptance NOT TESTED";
    }
    void nativeAtSpiListenerLifecycle100() {
        // Requires an isolated accessibility bus supplied by the test runner.
        // Never start/stop the user's accessibility service as part of this test.
        if(qEnvironmentVariable("WS_M6_NATIVE_ATSPI")!="1")
            QSKIP("NOT TESTED — set WS_M6_NATIVE_ATSPI=1 only in the controlled accessibility runtime harness.");
        auto session=createAtSpiSession();auto* transport=session.get();
        MonitorStatus lastState;int transitions=0;
        connect(transport,&IAtSpiSession::state,this,[&](const MonitorStatus& state){lastState=state;++transitions;});
        AtSpiSelectionMonitor monitor(std::move(session));
        for(int cycle=0;cycle<=100;++cycle) {
            const int before=transitions;QVERIFY(monitor.start());
            QTRY_VERIFY_WITH_TIMEOUT(transitions>before && lastState.running,5000);QVERIFY(monitor.status().running);
            const int running=transitions;monitor.stop();
            QTRY_VERIFY_WITH_TIMEOUT(transitions>running && !lastState.running,5000);QVERIFY(!monitor.latestSelection());
            drain();if(cycle==0 || cycle%20==0)sample("native_atspi_listener",cycle);
        }
        qInfo()<<"M6 native_ATSPI_listener_restarts=100 warmup=1; bus disappearance/real app exit recovery/real GNOME acceptance NOT TESTED";
    }
};

int main(int argc,char** argv) {
    qputenv("QT_NO_GLIB","1");XInitThreads();QApplication app(argc,argv);
    TestLinuxStability test;return QTest::qExec(&test,argc,argv);
}
#include "TestLinuxStability.moc"
