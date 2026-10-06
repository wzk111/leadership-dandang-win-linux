#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QPushButton>
#include "app/Application.h"
#include "ai/OpenAIProvider.h"
#include "platform/ClipboardSelectionProvider.h"
using namespace ws;
class ManualSecret : public ISecretStore {
public:
    int reads=0;
    bool read() override { ++reads; QTimer::singleShot(0,this,[this]{emit readFinished({"fake-key",{}});}); return true;}
    bool save(const QString&) override {return false;} bool remove() override {return false;}
    bool busy() const override {return false;} QString description() const override {return "fake";}
};
class ManualResolver : public IExplicitSelectionResolver {
public:
    int reads=0; std::optional<ResolvedSelection> value;
    std::optional<ResolvedSelection> resolve() override {++reads; return value;}
};
class ManualShortcut : public IGlobalShortcut {
public:
    int starts=0,stops=0; bool prompted=false;
    void start(bool explicitEnable) override {++starts; prompted=explicitEnable;}
    void stop() override {++stops;}
    GlobalShortcutStatus status() const override {return {};}
};
template<class T> T* findWindow() {for(auto* w:QApplication::topLevelWidgets()) if(auto* t=qobject_cast<T*>(w)) return t; return nullptr;}
class TestManualFlow : public QObject {
    Q_OBJECT
private slots:
    void shortcutSnapshotHttpAndEmpty() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
        Settings{"test-model","English"}.save(settings);
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        int requests=0; QByteArray received; QTcpSocket* pending=nullptr;
        connect(&server,&QTcpServer::newConnection,&server,[&] {
            auto* s=server.nextPendingConnection(); s->setParent(&server);
            connect(s,&QTcpSocket::readyRead,&server,[&,s] {
                received+=s->readAll();
                if(received.contains("\r\n\r\n") && received.endsWith('}') && !pending) {++requests; pending=s;}
            });
        });
        ManualSecret secrets; ManualResolver resolver; ManualShortcut shortcut;
        ClipboardSelectionProvider clipboard(*QApplication::clipboard());
        OpenAIProvider ai(nullptr,QUrl(QString("http://127.0.0.1:%1/v1/responses").arg(server.serverPort())));
        AppController controller(ai,secrets,clipboard,settings);
        Application app(controller,secrets,settings,nullptr,nullptr,&resolver,&shortcut); app.start();
        QApplication::setQuitOnLastWindowClosed(false);
        auto* palette=findWindow<ManualActionPalette>(); QVERIFY(palette);
        auto* settingsWindow=findWindow<SettingsWindow>(); QVERIFY(settingsWindow);
        auto* toggle=settingsWindow->findChild<QCheckBox*>("globalShortcut"); QVERIFY(toggle);
        QVERIFY(!toggle->isChecked()); QCOMPARE(shortcut.starts,0);
        toggle->setChecked(true); QCOMPARE(shortcut.starts,1); QVERIFY(shortcut.prompted);
        QVERIFY(settings.value("shortcut/enabled").toBool()); QVERIFY(!settings.value("ui/automaticPopup",false).toBool());
        QApplication::clipboard()->setText("OLD CLIPBOARD TEXT");
        resolver.value=ResolvedSelection{{"A",{},"X11 PRIMARY"},SelectionSource::PrimarySelection};
        emit shortcut.activated(); QVERIFY(palette->isVisible());
        QTest::qWait(100); QCOMPARE(requests,0); QCOMPARE(secrets.reads,0);
        resolver.value->selection.text="PRIMARY TEXT";
        emit shortcut.activated(); QCOMPARE(resolver.reads,2);
        resolver.value->selection.text="changed after trigger";
        palette->findChild<QPushButton*>("manualAction2")->click();
        QTRY_COMPARE(requests,1); QCOMPARE(secrets.reads,1);
        QVERIFY(received.contains("PRIMARY TEXT")); QVERIFY(!received.contains("OLD CLIPBOARD TEXT"));
        QVERIFY(!received.contains("changed after trigger")); QCOMPARE(resolver.reads,2);
        QCOMPARE(QApplication::clipboard()->text(),QString("OLD CLIPBOARD TEXT"));
        emit shortcut.activated(); QVERIFY(!palette->currentSelection()); QCOMPARE(resolver.reads,2);
        palette->findChild<QPushButton*>("manualAction2")->click(); QCOMPARE(requests,1);
        const QByteArray body=R"({"status":"completed","output":[{"type":"message","content":[{"type":"output_text","text":"result"}]}]})";
        pending->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body); pending->disconnectFromHost();
        QTRY_VERIFY(!controller.busy());
        app.triggerManualActions(); QVERIFY(palette->currentSelection());
        resolver.value.reset(); app.triggerManualActions(); QVERIFY(!palette->currentSelection());
        palette->findChild<QPushButton*>("manualAction2")->click(); QCOMPARE(requests,1);
        toggle->setChecked(false); QVERIFY(shortcut.stops>0);
        QVERIFY(!settings.value("shortcut/enabled").toBool());
    }
};
QTEST_MAIN(TestManualFlow)
#include "TestManualFlow.moc"
