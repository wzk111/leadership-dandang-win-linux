#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include "app/Application.h"
#include "ai/OpenAIProvider.h"
#include "platform/ClipboardSelectionProvider.h"
using namespace ws;
class MemorySecret : public ISecretStore {
public:
    QString key = "synthetic-key";
    bool read() override { QTimer::singleShot(0, this, [this] { emit readFinished({key, {}}); }); return true; }
    bool save(const QString& value) override { key = value; QTimer::singleShot(0, this, [this] { emit writeFinished(true, "Saved"); }); return true; }
    bool remove() override { key.clear(); return true; }
    bool busy() const override { return false; }
    QString description() const override { return "Test memory store"; }
};
class TestFlow : public QObject {
    Q_OBJECT
private slots:
    void clipboardToHttpToCopy() {
        QTemporaryDir dir; QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
        Settings{"test-model", "English"}.save(settings);
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        int requests = 0; QByteArray received;
        connect(&server, &QTcpServer::newConnection, &server, [&] {
            auto* socket = server.nextPendingConnection(); socket->setParent(&server);
            connect(socket, &QTcpSocket::readyRead, &server, [&, socket] {
                received += socket->readAll();
                if (!received.contains("\r\n\r\n") || !received.endsWith('}')) return;
                ++requests;
                const QByteArray body = R"({"status":"completed","output":[{"type":"message","content":[{"type":"output_text","text":"Clear synthetic result."}]}]})";
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
        MemorySecret secrets;
        ClipboardSelectionProvider clipboard(*QApplication::clipboard());
        OpenAIProvider ai(nullptr, QUrl(QString("http://127.0.0.1:%1/v1/responses").arg(server.serverPort())));
        AppController controller(ai, secrets, clipboard, settings);
        Application app(controller, secrets, settings); app.start();
        QApplication::setQuitOnLastWindowClosed(false);
        WorkspaceWindow* workspace = nullptr; ResultCard* result = nullptr; SettingsWindow* settingsWindow = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            if (auto* found = qobject_cast<WorkspaceWindow*>(w)) workspace = found;
            if (auto* found = qobject_cast<ResultCard*>(w)) result = found;
            if (auto* found = qobject_cast<SettingsWindow*>(w)) settingsWindow = found;
        }
        QVERIFY(workspace); QVERIFY(result); QVERIFY(settingsWindow);
        QCOMPARE(requests, 0);
        QApplication::clipboard()->setText("Synthetic source text.");
        workspace->findChild<QPushButton*>("processClipboard")->click();
        QCOMPARE(requests, 0);
        workspace->findChild<QPushButton*>("feature0")->click();
        QTRY_VERIFY(result->findChild<QPushButton*>("copyResult")->isEnabled());
        QCOMPARE(requests, 1);
        QVERIFY(received.contains("Synthetic source text."));
        result->findChild<QPushButton*>("copyResult")->click();
        QCOMPARE(QApplication::clipboard()->text(), QString("Clear synthetic result."));
        app.openSettings(); QVERIFY(settingsWindow->isVisible());
        settingsWindow->findChild<QLineEdit*>("apiKey")->setText("new-synthetic-key");
        settingsWindow->findChild<QPushButton*>("saveKey")->click();
        QVERIFY(settingsWindow->findChild<QLineEdit*>("apiKey")->text().isEmpty());
        QVERIFY(secrets.key == "new-synthetic-key");
        for (const auto& key : settings.allKeys()) QVERIFY(!settings.value(key).toString().contains("synthetic-key"));
        app.openDiagnostics();
    }
};
QTEST_MAIN(TestFlow)
#include "TestFlow.moc"
