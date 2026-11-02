#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTextStream>
#include <algorithm>
#include "app/Application.h"
#include "app/InstanceCoordinator.h"
#include "ai/OpenAIProvider.h"
#include "ai/AnthropicProvider.h"
#include "ai/GeminiProvider.h"
#include "ai/OpenAICompatibleProvider.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

using namespace ws;
namespace {
// Samples are observations, not a leak-free assertion. Warmup precedes each baseline.
void resources(const char* scenario, int completed) {
    qint64 resident=-1, privateBytes=-1, handles=-1, threads=-1;
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb=sizeof(memory);
    if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory))) {
        resident=qint64(memory.WorkingSetSize); privateBytes=qint64(memory.PrivateUsage);
    }
    DWORD count=0;if(GetProcessHandleCount(GetCurrentProcess(),&count))handles=count;
    const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snapshot!=INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry{};entry.dwSize=sizeof(entry);threads=0;
        if(Thread32First(snapshot,&entry))do {if(entry.th32OwnerProcessID==GetCurrentProcessId())++threads;}while(Thread32Next(snapshot,&entry));
        CloseHandle(snapshot);
    }
#elif defined(Q_OS_LINUX)
    QFile status("/proc/self/status");
    if(status.open(QIODevice::ReadOnly)) {
        const auto data=QString::fromLatin1(status.readAll());
        const auto rss=QRegularExpression("VmRSS:\\s+(\\d+)").match(data);
        const auto task=QRegularExpression("Threads:\\s+(\\d+)").match(data);
        if(rss.hasMatch())resident=rss.captured(1).toLongLong()*1024;
        if(task.hasMatch())threads=task.captured(1).toLongLong();
    }
    handles=QDir("/proc/self/fd").entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size();
#endif
    qInfo().nospace()<<"M6 resource scenario="<<scenario<<" completed="<<completed
        <<" rss_bytes="<<resident<<" private_bytes="<<privateBytes
        <<" handles_or_fds="<<handles<<" threads="<<threads;
}
void drainDeletes() { QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QCoreApplication::processEvents(); }

class SyntheticSecret final : public ISecretStore {
public:
    int reads=0;bool deferReads=false;
    bool read() override {++reads;if(!deferReads)QTimer::singleShot(0,this,[this]{finishRead();});return true;}
    void finishRead() {emit readFinished({"synthetic-key",{}});}
    bool save(const QString&) override {return false;}
    bool remove() override {return false;}
    bool busy() const override {return false;}
    QString description() const override {return "synthetic credential fixture";}
};
class SyntheticSelection final : public ISelectionProvider {
public: std::optional<Selection> currentSelection() override {return std::nullopt;}
};
class SyntheticMonitor final : public ISelectionMonitor {
public:
    MonitorStatus state;std::optional<Selection> selection;int starts=0,stops=0;
    bool start() override {++starts;state.available=state.initialized=state.running=true;emit statusChanged();return true;}
    void stop() override {++stops;state.running=false;clear();emit statusChanged();}
    MonitorStatus status() const override {return state;}
    std::optional<Selection> latestSelection() const override {return selection;}
    QString backendName() const override {return "M6 simulated selection events";}
    void detect(int n) {selection=Selection{QString("Synthetic selection %1.").arg(n),QRect(100,150,120,20),"synthetic fixture"};emit selectionDetected(*selection);}
    void clear() {selection.reset();emit selectionCleared();}
};
class SyntheticPolicy final : public IPlatformWindowPolicy {
public:
    OverlayCapability overlayCapability() const override {return OverlayCapability::AnchoredNonActivating;}
    bool configureActionBar(QWidget&) override {return true;}
    std::optional<BarPlacement> positionActionBar(QWidget& widget,const QRect& anchor) override {
        auto result=placeActionBar(anchor,widget.sizeHint(),QRect(0,0,1024,768));
        if(result)widget.move(result->geometry.topLeft());return result;
    }
    QString description() const override {return "simulated placement; no native focus evidence";}
};
class SyntheticShortcut final : public IGlobalShortcut {
public:
    void start(bool) override {}
    void stop() override {}
    GlobalShortcutStatus status() const override {return {};}
};
class SyntheticResolver final : public IExplicitSelectionResolver {
public:
    int reads=0;
    std::optional<ResolvedSelection> resolve() override {++reads;return ResolvedSelection{{"Synthetic manual selection.",{},"fixture"},SelectionSource::Clipboard};}
};
template<class T> T* window() {
    for(auto* widget:QApplication::topLevelWidgets())if(auto* result=qobject_cast<T*>(widget))return result;
    return nullptr;
}

// Read only enough to identify complete synthetic requests; do not retain/log bodies.
class CancellationServer final : public QTcpServer {
public:
    int requests=0;bool respond=false;
    QByteArray responseBody=R"({"status":"completed","output":[{"type":"message","content":[{"type":"output_text","text":"synthetic result"}]}]})";
    CancellationServer() {
        connect(this,&QTcpServer::newConnection,this,[this] {
            while(hasPendingConnections()) {
                auto* socket=nextPendingConnection();socket->setParent(this);
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
                connect(socket,&QTcpSocket::readyRead,this,[this,socket] {
                    if(socket->property("done").toBool()) {socket->readAll();return;}
                    const auto data=socket->property("buffer").toByteArray()+socket->readAll();
                    const auto split=data.indexOf("\r\n\r\n");
                    if(split<0) {socket->setProperty("buffer",data);return;}
                    const auto length=QRegularExpression("content-length:\\s*(\\d+)",QRegularExpression::CaseInsensitiveOption)
                        .match(QString::fromLatin1(data.left(split)));
                    if(!length.hasMatch() || data.size()-split-4<length.captured(1).toInt()) {socket->setProperty("buffer",data);return;}
                    socket->setProperty("done",true);socket->setProperty("buffer",QByteArray{});++requests;
                    if(respond) {
                        socket->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(responseBody.size())+"\r\nConnection: close\r\n\r\n"+responseBody);
                        socket->disconnectFromHost();
                    }
                });
            }
        });
    }
    QUrl endpoint(const QString& suffix) const {return QUrl(QString("http://127.0.0.1:%1/v1/%2").arg(serverPort()).arg(suffix));}
};
// HttpAIProvider owns a parentless QNetworkAccessManager member, so searching
// the provider's descendants cannot observe its replies. Capture manager child
// lifetimes without changing production ownership or exposing its private API.
class ReplyLifetimeProbe final : public QObject {
    struct Child {QPointer<QObject> object;bool reply=false;};
    QList<Child> children_;
    void identifyReplies() {
        for(auto& child:children_)if(child.object && qobject_cast<QNetworkReply*>(child.object.data()))child.reply=true;
    }
public:
    ReplyLifetimeProbe() {qApp->installEventFilter(this);}
    ~ReplyLifetimeProbe() override {qApp->removeEventFilter(this);}
    int createdReplies() {identifyReplies();int count=0;for(const auto& child:children_)if(child.reply)++count;return count;}
    int liveReplies() {identifyReplies();int count=0;for(const auto& child:children_)if(child.reply && child.object)++count;return count;}
    QPointer<QObject> lastReply() {
        identifyReplies();for(auto it=children_.crbegin();it!=children_.crend();++it)if(it->reply)return it->object;return {};
    }
protected:
    bool eventFilter(QObject* receiver,QEvent* event) override {
        if(event->type()==QEvent::ChildAdded && qobject_cast<QNetworkAccessManager*>(receiver)) {
            // ChildAdded can arrive during QObject's base constructor: defer
            // type identification until the child is fully constructed.
            children_.append({static_cast<QChildEvent*>(event)->child(),false});
        }
        // A very fast success can finish and deleteLater in one event-loop turn.
        // Identify it before DeferredDelete is delivered, while the dynamic type
        // and captured QPointer are still valid; after delivery the pointer clears.
        if(event->type()==QEvent::DeferredDelete && qobject_cast<QNetworkReply*>(receiver))
            for(auto& child:children_)if(child.object==receiver)child.reply=true;
        return false;
    }
};
std::unique_ptr<IAIProvider> provider(const QString& id,CancellationServer& server) {
    if(id=="anthropic")return std::make_unique<AnthropicProvider>(nullptr,server.endpoint("messages"),5000);
    if(id=="gemini")return std::make_unique<GeminiProvider>(nullptr,server.endpoint("interactions"),5000);
    if(id=="openai_compatible")return std::make_unique<OpenAICompatibleProvider>(nullptr,5000,true);
    return std::make_unique<OpenAIProvider>(nullptr,server.endpoint("responses"),5000);
}
struct StopProcesses {
    QProcess &a,&b;
    ~StopProcesses(){for(auto* p:{&a,&b})if(p->state()!=QProcess::NotRunning){p->kill();p->waitForFinished(3000);}}
};
}

class TestStability final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QApplication::setQuitOnLastWindowClosed(false);
        qInfo()<<"M6 Level A synthetic stability"<<QSysInfo::prettyProductName()<<"Qt"<<qVersion()
            <<"backend"<<QGuiApplication::platformName()<<"cpu"<<QSysInfo::currentCpuArchitecture()
            <<"session"<<qEnvironmentVariable("XDG_SESSION_TYPE","unavailable")
            <<"desktop"<<qEnvironmentVariable("XDG_CURRENT_DESKTOP","unavailable");
    }
    void simulatedSelectionToolbarAndManualLifecycle_data() {
        QTest::addColumn<QString>("id");
        for(const auto* id:{"openai","anthropic","gemini","openai_compatible"})QTest::newRow(id)<<QString(id);
    }
    void simulatedSelectionToolbarAndManualLifecycle() {
        QFETCH(QString,id);
        QTemporaryDir dir;QVERIFY(dir.isValid());QSettings settings(dir.filePath("settings.ini"),QSettings::IniFormat);
        settings.setValue("ui/automaticPopup",true);
        CancellationServer server;QVERIFY(server.listen(QHostAddress::LocalHost));
        auto ai=provider(id,server);SyntheticSecret secret;SyntheticSelection selection;
        Settings config;config.model="synthetic-model";config.providerId=id;config.baseUrl=server.endpoint("").toString();QVERIFY(config.save(settings));
        AppController controller(*ai,secret,selection,settings);SyntheticMonitor monitor;SyntheticPolicy policy;
        SyntheticResolver resolver;SyntheticShortcut shortcut;
        Application app(controller,secret,settings,&monitor,&policy,&resolver,&shortcut);app.start(false);
        QApplication::setQuitOnLastWindowClosed(false);
        auto* bar=window<ActionBar>();auto* palette=window<ManualActionPalette>();QVERIFY(bar);QVERIFY(palette);
        for(int i=0;i<10;++i){monitor.detect(i);monitor.clear();emit shortcut.activated();palette->close();}
        drainDeletes();const int initialReads=resolver.reads;resources("simulated_ui",0);
        QList<qint64> latencies,manualLatencies;
        for(int cycle=1;cycle<=500;++cycle) {
            QElapsedTimer timer;timer.start();monitor.detect(cycle*2);QVERIFY(bar->isVisible());
            latencies<<timer.nsecsElapsed();
            // A second event must replace the snapshot while the same bar is shown.
            monitor.detect(cycle*2+1);QVERIFY(bar->currentSelection());
            QCOMPARE(bar->currentSelection()->text,monitor.latestSelection()->text);
            monitor.clear();QVERIFY(!bar->isVisible());QVERIFY(!bar->currentSelection());
            if(cycle%100==0){drainDeletes();resources("simulated_ui",cycle);}
        }
        for(int cycle=1;cycle<=200;++cycle) {
            QElapsedTimer timer;timer.start();
            emit shortcut.activated();QVERIFY(palette->isVisible());QVERIFY(palette->currentSelection());
            manualLatencies<<timer.nsecsElapsed();
            palette->close();QVERIFY(!palette->isVisible());
            if(cycle%50==0){drainDeletes();resources("simulated_shortcut_palette",cycle);}
        }
        QCOMPARE(resolver.reads-initialReads,200);
        // Opening the composer is an explicit local step, and still must not
        // read credentials or upload until Generate Reply is chosen.
        QVERIFY(controller.runSelection(Feature::Reply,{"Synthetic incoming message.",{},"fixture"}));
        auto* composer=window<ReplyComposer>();QVERIFY(composer);QVERIFY(composer->isVisible());QVERIFY(composer->currentSelection());
        drainDeletes();QCOMPARE(server.requests,0);QCOMPARE(secret.reads,0);QVERIFY(!controller.busy());
        composer->close();QVERIFY(!composer->isVisible());QVERIFY(!composer->currentSelection());
        for(int i=0;i<100;++i){monitor.stop();QVERIFY(!monitor.status().running);QVERIFY(monitor.start());}
        QCoreApplication::processEvents();QCOMPARE(server.requests,0);QCOMPARE(secret.reads,0);QVERIFY(!controller.busy());
        std::sort(latencies.begin(),latencies.end());
        std::sort(manualLatencies.begin(),manualLatencies.end());
        qInfo()<<"M6 simulated provider"<<id<<"counts selection_events=1000 actionbar_cycles=500 shortcut_activations=200 palette_cycles=200 monitor_restarts=100 HTTP=0"
            <<"selection-to-visible p95_ns"<<latencies[474]<<"samples"<<latencies.size()
            <<"shortcut-to-palette p95_ns"<<manualLatencies[189]<<"samples"<<manualLatencies.size()
            <<"reply_composer_open=1 HTTP=0"
            <<"boundary=direct fixture signal through Application and synchronous synthetic resolver; excludes native input/provider and window manager";
        resources("simulated_ui_final",500);
    }
    void syntheticHttpCancellation_data() {
        QTest::addColumn<QString>("id");
        for(const auto* id:{"openai","anthropic","gemini","openai_compatible"})QTest::newRow(id)<<QString(id);
    }
    void syntheticHttpCancellation() {
        QFETCH(QString,id);CancellationServer server;QVERIFY(server.listen(QHostAddress::LocalHost));auto ai=provider(id,server);
        ReplyLifetimeProbe replies;
        SyntheticSecret secret;SyntheticSelection selection;QTemporaryDir dir;QVERIFY(dir.isValid());
        QSettings settings(dir.filePath("settings.ini"),QSettings::IniFormat);
        Settings config;config.model="synthetic-model";config.providerId=id;
        config.baseUrl=server.endpoint("").toString();QVERIFY(config.save(settings));
        AppController controller(*ai,secret,selection,settings);Application app(controller,secret,settings);app.start(false);
        QApplication::setQuitOnLastWindowClosed(false);
        auto* result=window<ResultCard>();QVERIFY(result);auto* cancel=result->findChild<QPushButton*>("cancelRequest");QVERIFY(cancel);
        QSignalSpy finished(&controller,&AppController::finished);
        for(int cycle=0;cycle<=25;++cycle) {
            const int requests=server.requests;
            QVERIFY(controller.runSelection(Feature::Summarize,{"Synthetic cancellation selection.",{},"fixture"}));
            QTRY_COMPARE_WITH_TIMEOUT(server.requests,requests+1,3000);
            QCOMPARE(replies.createdReplies(),cycle+1);QCOMPARE(replies.liveReplies(),1);
            const auto reply=replies.lastReply();QVERIFY(reply);QVERIFY(qobject_cast<QNetworkReply*>(reply.data()));
            QVERIFY(controller.busy());QVERIFY(cancel->isEnabled());cancel->click();
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,3000);
            QCOMPARE(qvariant_cast<AIResult>(finished.takeFirst()[0]).error,AIError::Cancelled);
            QVERIFY(!controller.busy());QVERIFY(!ai->busy());QVERIFY(!cancel->isEnabled());
            QTRY_VERIFY_WITH_TIMEOUT(server.findChildren<QTcpSocket*>().isEmpty(),3000);
            QTRY_VERIFY_WITH_TIMEOUT(reply.isNull(),3000);
            drainDeletes();QCOMPARE(replies.liveReplies(),0);
            if(cycle==0 || cycle%5==0)resources(qPrintable(id),cycle);
        }
        QCOMPARE(server.requests,26);QCOMPARE(secret.reads,26);
        server.respond=true;
        if(id=="anthropic")server.responseBody=R"({"type":"message","role":"assistant","stop_reason":"end_turn","content":[{"type":"text","text":"synthetic result"}]})";
        if(id=="gemini")server.responseBody=R"({"status":"completed","steps":[{"type":"model_output","content":[{"type":"text","text":"synthetic result"}]}]})";
        QVERIFY(controller.runSelection(Feature::Summarize,{"Synthetic recovery selection.",{},"fixture"}));
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,3000);
        QCOMPARE(qvariant_cast<AIResult>(finished.takeFirst()[0]).error,AIError::None);QVERIFY(!controller.busy());
        QCOMPARE(server.requests,27);QCOMPARE(secret.reads,27);
        QTRY_VERIFY_WITH_TIMEOUT(server.findChildren<QTcpSocket*>().isEmpty(),3000);
        QTRY_COMPARE_WITH_TIMEOUT(replies.createdReplies(),27,3000);
        QTRY_COMPARE_WITH_TIMEOUT(replies.liveReplies(),0,3000);
        drainDeletes();
        qInfo()<<"M6 HTTP cancellation provider"<<id<<"measured=25 warmup=1 boundary=ResultCard Cancel to AppController to real loopback HTTP abort";
    }
    void pendingRequestAboutToQuit_data() {
        QTest::addColumn<QString>("id");QTest::addColumn<bool>("httpPending");
        for(const auto* id:{"openai","anthropic","gemini","openai_compatible"}) {
            QTest::newRow(qPrintable(QString(id)+"-pending-secret"))<<QString(id)<<false;
            QTest::newRow(qPrintable(QString(id)+"-pending-http"))<<QString(id)<<true;
        }
    }
    void pendingRequestAboutToQuit() {
        QFETCH(QString,id);QFETCH(bool,httpPending);
        CancellationServer server;QVERIFY(server.listen(QHostAddress::LocalHost));auto ai=provider(id,server);
        ReplyLifetimeProbe replies;SyntheticSecret secret;secret.deferReads=true;SyntheticSelection selection;
        QTemporaryDir dir;QVERIFY(dir.isValid());QSettings settings(dir.filePath("settings.ini"),QSettings::IniFormat);
        Settings config;config.model="synthetic-model";config.providerId=id;config.baseUrl=server.endpoint("").toString();QVERIFY(config.save(settings));
        AppController controller(*ai,secret,selection,settings);Application app(controller,secret,settings);app.start(false);
        QApplication::setQuitOnLastWindowClosed(false);
        QSignalSpy finished(&controller,&AppController::finished);
        QVERIFY(controller.runSelection(Feature::Summarize,{"Synthetic shutdown selection.",{},"fixture"}));
        QVERIFY(controller.busy());QCOMPARE(secret.reads,1);QCOMPARE(server.requests,0);
        QPointer<QObject> reply;
        if(httpPending) {
            secret.finishRead();QTRY_COMPARE_WITH_TIMEOUT(server.requests,1,3000);
            QCOMPARE(replies.createdReplies(),1);reply=replies.lastReply();QVERIFY(reply);
            QVERIFY(qobject_cast<QNetworkReply*>(reply.data()));QVERIFY(ai->busy());
        } else {QVERIFY(!ai->busy());QCOMPARE(replies.createdReplies(),0);}
        // Emit the real application shutdown signal without quitting the test
        // process; production Application's connection must cancel the controller.
        QElapsedTimer cleanup;cleanup.start();
        QVERIFY(QMetaObject::invokeMethod(qApp,"aboutToQuit",Qt::DirectConnection));
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,2000);
        QCOMPARE(qvariant_cast<AIResult>(finished[0][0]).error,AIError::Cancelled);
        QVERIFY(!controller.busy());QVERIFY(!ai->busy());
        if(httpPending)QTRY_VERIFY_WITH_TIMEOUT(reply.isNull(),2000);
        QTRY_VERIFY_WITH_TIMEOUT(server.findChildren<QTcpSocket*>().isEmpty(),2000);
        QVERIFY(cleanup.elapsed()<2000);
        // A delayed secret callback after shutdown must not generate a new
        // request or deliver a second completion, in either pending phase.
        secret.finishRead();drainDeletes();
        QCOMPARE(finished.size(),1);QCOMPARE(secret.reads,1);QCOMPARE(server.requests,httpPending?1:0);
        QCOMPARE(replies.createdReplies(),httpPending?1:0);QCOMPARE(replies.liveReplies(),0);
        QVERIFY(!controller.busy());QVERIFY(!ai->busy());
        qInfo()<<"M6 shutdown provider"<<id<<"pending_phase"<<(httpPending?"HTTP":"secret")
            <<"cleanup_ms"<<cleanup.elapsed()<<"late_secret_ignored=1 boundary=aboutToQuit signal through production Application connection; full process exit excluded";
    }
    void ipcStartupRacesAndRestarts() {
        QTemporaryDir dir;QVERIFY(dir.isValid());const auto ipc=dir.filePath("ipc");
        const auto exe=QCoreApplication::applicationFilePath();
        // One warmup followed by 100 measured simultaneous startup races. Each
        // winner is killed, then a fresh production coordinator recovers its endpoint.
        for(int cycle=0;cycle<=100;++cycle) {
            QProcess a,b;StopProcesses cleanup{a,b};
            a.start(exe,{"--stability-ipc-peer",ipc});b.start(exe,{"--stability-ipc-peer",ipc});
            QVERIFY(a.waitForStarted(3000));QVERIFY(b.waitForStarted(3000));QByteArray first,second;
            QTRY_VERIFY_WITH_TIMEOUT(([&] {
                first+=a.readAllStandardOutput();second+=b.readAllStandardOutput();
                return (first.contains("primary") || second.contains("primary")) &&
                    (a.state()==QProcess::NotRunning || b.state()==QProcess::NotRunning);
            })(),7000);
            QCOMPARE(first.count("primary")+second.count("primary"),1);
            auto* owner=first.contains("primary")?&a:&b;auto* loser=owner==&a?&b:&a;
            QCOMPARE(owner->state(),QProcess::Running);QCOMPARE(loser->exitStatus(),QProcess::NormalExit);
            QVERIFY(loser->exitCode()==0 || loser->exitCode()==2);
            if(cycle==1) {
                for(int trigger=0;trigger<20;++trigger) {
                    QProcess client;client.start(exe,{"--stability-ipc-peer",ipc,"--trigger"});
                    QVERIFY(client.waitForFinished(5000));QCOMPARE(client.exitStatus(),QProcess::NormalExit);QCOMPARE(client.exitCode(),0);
                }
                QByteArray received;
                QTRY_VERIFY_WITH_TIMEOUT(([&]{received+=owner->readAllStandardOutput();return received.count("trigger")==20;})(),3000);
            }
            owner->kill();QVERIFY(owner->waitForFinished(3000));
            QLocalSocket idle;
            {
                InstanceCoordinator recovered(ipc);QCOMPARE(recovered.start(false),InstanceCoordinator::Result::Primary);
                QSignalSpy triggered(&recovered,&InstanceCoordinator::triggerRequested);
                QLocalSocket invalid;invalid.connectToServer(recovered.endpoint());QVERIFY(invalid.waitForConnected(1000));
                invalid.write("invalid\n");invalid.flush();QTRY_COMPARE(invalid.state(),QLocalSocket::UnconnectedState);QCOMPARE(triggered.size(),0);
                QLocalSocket valid;valid.connectToServer(recovered.endpoint());QVERIFY(valid.waitForConnected(1000));
                valid.write("trigger\n");valid.flush();QTRY_COMPARE(triggered.size(),1);
                QTRY_VERIFY(valid.bytesAvailable()>0);QCOMPARE(valid.readAll(),QByteArray("ok\n"));
                // Keep this peer alive across primary destruction, with an
                // incomplete command to model forwarding during shutdown.
                idle.connectToServer(recovered.endpoint());QVERIFY(idle.waitForConnected(1000));
                idle.write("trig");idle.flush();
                QCoreApplication::processEvents();
            }
            QTRY_COMPARE_WITH_TIMEOUT(idle.state(),QLocalSocket::UnconnectedState,2000);
            drainDeletes();if(cycle==0 || cycle%20==0)resources("ipc_race_restart",cycle);
        }
        qInfo()<<"M6 IPC measured_startup_races=100 crash_recovery_restarts=100 rapid_trigger_calls=20; synthetic current-user subprocesses; cross-user/session isolation NOT TESTED";
    }
};

int main(int argc,char** argv) {
    if(argc>1 && QByteArray(argv[1])=="--stability-ipc-peer") {
        QCoreApplication app(argc,argv);InstanceCoordinator instance(app.arguments().value(2));
        QObject::connect(&instance,&InstanceCoordinator::triggerRequested,&app,[]{QTextStream(stdout)<<"trigger"<<Qt::endl;});
        const auto result=instance.start(app.arguments().contains("--trigger"));
        if(result==InstanceCoordinator::Result::Forwarded)return 0;
        if(result==InstanceCoordinator::Result::Failed)return 2;
        QTextStream(stdout)<<"primary"<<Qt::endl;QTimer::singleShot(15000,&app,&QCoreApplication::quit);return app.exec();
    }
    QApplication app(argc,argv);TestStability test;return QTest::qExec(&test,argc,argv);
}
#include "TestStability.moc"
