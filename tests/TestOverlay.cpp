#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include "app/Application.h"
#include "ui/ActionBar.h"
#include "ai/OpenAIProvider.h"
#include "platform/ClipboardSelectionProvider.h"
using namespace ws;
class OverlaySecret : public ISecretStore {
public:
    int reads=0;
    bool read() override { ++reads; QTimer::singleShot(0,this,[this]{ emit readFinished({"synthetic-key",{}}); }); return true; }
    bool save(const QString&) override { return false; }
    bool remove() override { return false; }
    bool busy() const override { return false; }
    QString description() const override { return "test"; }
};
class OverlayMonitor : public ISelectionMonitor {
public:
    MonitorStatus state; std::optional<Selection> value; int starts=0; int stops=0;
    bool start() override { ++starts; state.running=true; state.available=true; emit statusChanged(); return true; }
    void stop() override { ++stops; state.running=false; value.reset(); emit selectionCleared(); emit statusChanged(); }
    MonitorStatus status() const override { return state; }
    std::optional<Selection> latestSelection() const override { return value; }
    QString backendName() const override { return "test"; }
    void detect(QString text, std::optional<QRect> anchor=QRect(300,200,100,20)) {
        value=Selection{text,anchor,"synthetic app"}; emit selectionDetected(*value);
    }
    void clear() { value.reset(); emit selectionCleared(); }
};
class OverlayPolicy : public IPlatformWindowPolicy {
public:
    bool supported=true;
    OverlayCapability overlayCapability() const override { return supported?OverlayCapability::AnchoredNonActivating:OverlayCapability::Unsupported; }
    bool configureActionBar(QWidget&) override { return supported; }
    std::optional<BarPlacement> positionActionBar(QWidget& w,const QRect& a) override {
        auto p=placeActionBar(a,w.sizeHint(),{0,0,800,600}); if(p) w.move(p->geometry.topLeft()); return p;
    }
    QString description() const override { return supported?"AnchoredNonActivating":"Unsupported: native Wayland"; }
};
template<class T> T* top() {
    for(auto* w:QApplication::topLevelWidgets()) if(auto* result=qobject_cast<T*>(w)) return result;
    return nullptr;
}
class TestOverlay : public QObject {
    Q_OBJECT
private slots:
    void togglesLifecycleAndMissingAnchor() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
        OverlaySecret secret; ClipboardSelectionProvider clipboard(*QApplication::clipboard()); OpenAIProvider ai;
        AppController controller(ai,secret,clipboard,settings); OverlayMonitor monitor; OverlayPolicy policy;
        Application app(controller,secret,settings,&monitor,&policy); app.start();
        QApplication::setQuitOnLastWindowClosed(false);
        auto* bar=top<ActionBar>(); QVERIFY(bar);
        auto* settingsWindow=top<SettingsWindow>(); QVERIFY(settingsWindow);
        auto* toggle=settingsWindow->findChild<QCheckBox*>("automaticPopup"); QVERIFY(toggle);
        QVERIFY(!toggle->isChecked());
        monitor.detect("A"); QVERIFY(!bar->isVisible()); QCOMPARE(secret.reads,0);
        toggle->setChecked(true);
        QCOMPARE(settings.value("ui/automaticPopup").toBool(),true);
        monitor.detect("B"); QVERIFY(bar->isVisible()); QCOMPARE(bar->currentSelection()->text,QString("B"));
        QAction* trayToggle=nullptr;
        for(auto* w:QApplication::topLevelWidgets())
            if(auto* m=qobject_cast<QMenu*>(w)) for(auto* a:m->actions()) if(a->objectName()=="automaticToolbar") trayToggle=a;
        QVERIFY(trayToggle); QVERIFY(trayToggle->isChecked());
        trayToggle->setChecked(false);
        QVERIFY(!toggle->isChecked()); QVERIFY(!bar->isVisible()); QVERIFY(!bar->currentSelection());
        QCOMPARE(monitor.stops,0); QVERIFY(monitor.status().running);
        trayToggle->setChecked(true); QVERIFY(toggle->isChecked());
        monitor.detect("missing anchor",{}); QVERIFY(!bar->isVisible()); QVERIFY(monitor.latestSelection());
        app.openDiagnostics();
        bool found=false;
        for(auto* w:QApplication::topLevelWidgets())
            for(auto* t:w->findChildren<QPlainTextEdit*>())
                if(t->objectName()=="overlayMetadata") {
                    found=true; QVERIFY(t->toPlainText().contains("no anchor rectangle"));
                    QVERIFY(!t->toPlainText().contains("missing anchor"));
                }
        QVERIFY(found);
        monitor.detect("C"); QVERIFY(bar->isVisible()); monitor.clear();
        QVERIFY(!bar->isVisible()); QVERIFY(!bar->currentSelection());
        bar->findChild<QPushButton*>("action0")->click(); QCOMPARE(secret.reads,0);
        monitor.detect("D"); QVERIFY(bar->isVisible());
        monitor.state.running=false; emit monitor.statusChanged();
        QVERIFY(!bar->isVisible()); QVERIFY(!bar->currentSelection());
        QSettings reread(dir.filePath("s.ini"),QSettings::IniFormat);
        QVERIFY(reread.value("ui/automaticPopup").toBool());
    }
    void unsupportedPolicy() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
        settings.setValue("ui/automaticPopup",true);
        OverlaySecret secret; ClipboardSelectionProvider clipboard(*QApplication::clipboard()); OpenAIProvider ai;
        AppController controller(ai,secret,clipboard,settings); OverlayMonitor monitor; OverlayPolicy policy; policy.supported=false;
        Application app(controller,secret,settings,&monitor,&policy); app.start();
        QApplication::setQuitOnLastWindowClosed(false);
        auto* bar=top<ActionBar>(); QVERIFY(bar); monitor.detect("source");
        QVERIFY(!bar->isVisible()); QCOMPARE(secret.reads,0); QVERIFY(monitor.latestSelection());
        QApplication::clipboard()->setText("manual fallback");
        QVERIFY(controller.captureClipboard());
    }
    void explicitClickHttpSnapshotAndBusy() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
        Settings{"test-model","English"}.save(settings); settings.setValue("ui/automaticPopup",true);
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        int requests=0; QByteArray received; QTcpSocket* pending=nullptr;
        connect(&server,&QTcpServer::newConnection,&server,[&] {
            auto* s=server.nextPendingConnection(); s->setParent(&server);
            connect(s,&QTcpSocket::readyRead,&server,[&,s] {
                received+=s->readAll();
                if(received.contains("\r\n\r\n") && received.endsWith('}') && !pending) { ++requests; pending=s; }
            });
        });
        OverlaySecret secret; ClipboardSelectionProvider clipboard(*QApplication::clipboard());
        OpenAIProvider ai(nullptr,QUrl(QString("http://127.0.0.1:%1/v1/responses").arg(server.serverPort())));
        AppController controller(ai,secret,clipboard,settings); OverlayMonitor monitor; OverlayPolicy policy;
        Application app(controller,secret,settings,&monitor,&policy); app.start();
        QApplication::setQuitOnLastWindowClosed(false);
        auto* bar=top<ActionBar>(); QVERIFY(bar);
        QApplication::clipboard()->setText("UNRELATED CLIPBOARD");
        monitor.detect("STALE SOURCE A"); QVERIFY(bar->isVisible());
        monitor.detect("SOURCE FROM ATSPI B"); QCOMPARE(bar->currentSelection()->text,QString("SOURCE FROM ATSPI B"));
        // Yield the event loop: a scheduled automatic upload would now be observable.
        QTest::qWait(150); QCOMPARE(requests,0); QCOMPARE(secret.reads,0); QVERIFY(!controller.busy());
        QTest::mouseClick(bar->findChild<QPushButton*>("action2"),Qt::LeftButton);
        QTRY_COMPARE(requests,1);
        QVERIFY(received.contains("SOURCE FROM ATSPI B")); QVERIFY(!received.contains("STALE SOURCE A"));
        QVERIFY(!received.contains("UNRELATED CLIPBOARD"));
        QCOMPARE(QApplication::clipboard()->text(),QString("UNRELATED CLIPBOARD"));
        QVERIFY(controller.busy()); QVERIFY(!bar->currentSelection());
        monitor.detect("WHILE BUSY"); QVERIFY(!bar->isVisible());
        bar->findChild<QPushButton*>("action0")->click(); QCOMPARE(requests,1); QCOMPARE(secret.reads,1);
        const QByteArray body=R"({"status":"completed","output":[{"type":"message","content":[{"type":"output_text","text":"explicit result"}]}]})";
        QVERIFY(pending); pending->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body); pending->disconnectFromHost();
        QTRY_VERIFY(!controller.busy());
        auto* result=top<ResultCard>(); QVERIFY(result);
        QTRY_VERIFY(result->findChild<QPushButton*>("copyResult")->isEnabled());
        QCOMPARE(QApplication::clipboard()->text(),QString("UNRELATED CLIPBOARD"));
        result->findChild<QPushButton*>("copyResult")->click();
        QCOMPARE(QApplication::clipboard()->text(),QString("explicit result"));
        monitor.detect("CLEAR BEFORE CLICK"); QVERIFY(bar->isVisible()); monitor.clear();
        bar->findChild<QPushButton*>("action0")->click();
        QTest::qWait(100); QCOMPARE(requests,1); QCOMPARE(secret.reads,1);
        for(const auto& key:settings.allKeys()) QVERIFY(!settings.value(key).toString().contains("SOURCE FROM ATSPI"));
    }
};
QTEST_MAIN(TestOverlay)
#include "TestOverlay.moc"
