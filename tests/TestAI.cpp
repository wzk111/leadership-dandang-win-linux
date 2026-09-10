#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include "ai/OpenAIProvider.h"
using namespace ws;
static const QByteArray successBody = R"({"status":"completed","output":[{"type":"reasoning"},{"type":"message","role":"assistant","content":[{"type":"output_text","text":"Hello"},{"type":"output_text","text":"world"}]}]})";
class TestAI : public QObject {
    Q_OBJECT
private slots:
    void decodeSuccess() {
        const auto r = OpenAIProvider::decode(200, QNetworkReply::NoError, successBody);
        QCOMPARE(r.error, AIError::None); QCOMPARE(r.text, QString("Hello\nworld"));
    }
    void errors_data() {
        QTest::addColumn<int>("status"); QTest::addColumn<QByteArray>("body"); QTest::addColumn<int>("expected");
        QTest::newRow("auth") << 401 << QByteArray("PRIVATE SECRET") << int(AIError::Authentication);
        QTest::newRow("forbidden") << 403 << QByteArray("PRIVATE SECRET") << int(AIError::Authentication);
        QTest::newRow("rate-limit") << 429 << QByteArray("PRIVATE SECRET") << int(AIError::RateLimited);
        QTest::newRow("server") << 500 << QByteArray("PRIVATE SECRET") << int(AIError::Server);
        QTest::newRow("json") << 200 << QByteArray("PRIVATE SECRET") << int(AIError::InvalidJson);
        QTest::newRow("array") << 200 << QByteArray("[]") << int(AIError::InvalidJson);
        QTest::newRow("empty") << 200 << QByteArray(R"({"status":"completed","output":[]})") << int(AIError::EmptyResponse);
        QTest::newRow("incomplete") << 200 << QByteArray(R"({"status":"incomplete","output":[]})") << int(AIError::Incomplete);
        QTest::newRow("refused") << 200 << QByteArray(R"({"status":"completed","output":[{"type":"message","content":[{"type":"refusal","refusal":"PRIVATE SECRET"}]}]})") << int(AIError::Refused);
    }
    void errors() {
        QFETCH(int, status); QFETCH(QByteArray, body); QFETCH(int, expected);
        const auto r = OpenAIProvider::decode(status, QNetworkReply::NoError, body);
        QCOMPARE(int(r.error), expected); QVERIFY(r.text.isEmpty());
        QVERIFY(!r.message.isEmpty()); QVERIFY(!r.message.contains("PRIVATE SECRET"));
    }
    void networkFailure() {
        QCOMPARE(OpenAIProvider::decode(0, QNetworkReply::HostNotFoundError, {}).error, AIError::Network);
    }
    void validation() {
        OpenAIProvider p; QSignalSpy spy(&p, &IAIProvider::completed);
        QVERIFY(p.generate({{"instruction", "input", {}}, "model", ""}));
        QTRY_COMPARE(spy.size(), 1);
        QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error, AIError::MissingKey);
        QVERIFY(p.generate({{"instruction", "input", {}}, "", "test-key"}));
        QTRY_COMPARE(spy.size(), 2);
        QCOMPARE(qvariant_cast<AIResult>(spy[1][0]).error, AIError::MissingModel);
    }
    void asyncRequest() {
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray captured;
        connect(&server, &QTcpServer::newConnection, &server, [&] {
            auto* s = server.nextPendingConnection(); s->setParent(&server);
            connect(s, &QTcpSocket::readyRead, &server, [&, s] {
                captured += s->readAll();
                const int split = captured.indexOf("\r\n\r\n");
                if (split < 0 || !captured.endsWith('}')) return;
                s->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(successBody.size()) + "\r\nConnection: close\r\n\r\n" + successBody);
                s->disconnectFromHost();
            });
        });
        OpenAIProvider p(nullptr, QUrl(QString("http://127.0.0.1:%1/v1/responses").arg(server.serverPort())));
        QSignalSpy spy(&p, &IAIProvider::completed);
        QVERIFY(p.generate({{"instruction", "selected text", {}}, "configured-model", "test-key"}));
        QVERIFY(p.busy()); QVERIFY(!p.generate({}));
        QTRY_COMPARE(spy.size(), 1); QVERIFY(!p.busy());
        QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).text, QString("Hello\nworld"));
        QVERIFY(captured.startsWith("POST /v1/responses "));
        const auto json = QJsonDocument::fromJson(captured.mid(captured.indexOf("\r\n\r\n") + 4)).object();
        QCOMPARE(json["model"].toString(), QString("configured-model"));
        QCOMPARE(json["input"].toString(), QString("selected text"));
        QVERIFY(json.contains("store")); QVERIFY(!json["store"].toBool());
        QVERIFY(!json.contains("temperature"));
    }
    void timeoutAndCancel() {
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        OpenAIProvider p(nullptr, QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort())), 40);
        QSignalSpy spy(&p, &IAIProvider::completed);
        AIRequest req{{"instruction", "input", {}}, "model", "test-key"};
        QVERIFY(p.generate(req)); QTRY_COMPARE(spy.size(), 1);
        QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error, AIError::Timeout);
        QVERIFY(!p.busy()); QVERIFY(p.generate(req)); p.cancel();
        QTRY_COMPARE(spy.size(), 2);
        QCOMPARE(qvariant_cast<AIResult>(spy[1][0]).error, AIError::Cancelled);
        QVERIFY(!p.busy());
    }
};
QTEST_GUILESS_MAIN(TestAI)
#include "TestAI.moc"
