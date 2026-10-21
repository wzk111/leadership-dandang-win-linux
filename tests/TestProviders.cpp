#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include "app/AppController.h"
#include "core/Profile.h"
#include "ai/OpenAIProvider.h"
#include "ai/AnthropicProvider.h"
#include "ai/GeminiProvider.h"
#include "ai/OpenAICompatibleProvider.h"
#include "ai/AIService.h"
using namespace ws;
class Server:public QTcpServer {
public:
    QByteArray captured,body;int status=200;int requests=0;bool hang=false;
    Server() {
        listen(QHostAddress::LocalHost);
        connect(this,&QTcpServer::newConnection,this,[this] {
            auto* socket=nextPendingConnection();socket->setParent(this);
            connect(socket,&QTcpSocket::readyRead,this,[this,socket] {
                auto data=socket->property("data").toByteArray()+socket->readAll();socket->setProperty("data",data);
                const int split=data.indexOf("\r\n\r\n");if(split<0 || socket->property("done").toBool())return;
                const auto match=QRegularExpression("content-length: (\\d+)",QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(data.left(split)));
                if(!match.hasMatch() || data.size()-split-4<match.captured(1).toInt())return;
                socket->setProperty("done",true);captured=data;++requests;if(hang)return;
                socket->write("HTTP/1.1 "+QByteArray::number(status)+" Result\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
            });
        });
    }
    QUrl base()const{return QUrl(QString("http://127.0.0.1:%1/v1").arg(serverPort()));}
};
static QByteArray success(const QString& id) {
    if(id=="anthropic")return R"({"type":"message","role":"assistant","stop_reason":"end_turn","content":[{"type":"thinking","thinking":"PRIVATE THOUGHT"},{"type":"text","text":"visible answer"}]})";
    if(id=="gemini")return R"({"status":"completed","steps":[{"type":"thought","summary":[{"type":"text","text":"PRIVATE THOUGHT"}]},{"type":"user_input","content":[{"type":"text","text":"PRIVATE INPUT"}]},{"type":"model_output","content":[{"type":"text","text":"visible answer"}]}]})";
    return R"({"status":"completed","output":[{"type":"message","role":"assistant","content":[{"type":"output_text","text":"visible answer"}]}]})";
}
static std::unique_ptr<IAIProvider> make(const QString& id,Server& s,int timeout=1000) {
    if(id=="anthropic")return std::make_unique<AnthropicProvider>(nullptr,QUrl(s.base().toString()+"/messages"),timeout);
    if(id=="gemini")return std::make_unique<GeminiProvider>(nullptr,QUrl(s.base().toString()+"/interactions"),timeout);
    if(id=="openai_compatible")return std::make_unique<OpenAICompatibleProvider>(nullptr,timeout,true);
    return std::make_unique<OpenAIProvider>(nullptr,QUrl(s.base().toString()+"/responses"),timeout);
}
static AIRequest request(const QString& id,Server& s){return {{"system task","source text",{}},"configured-model","synthetic-key",1234,id,s.base().toString()};}
class HTTPSecret:public ISecretStore {
public:
    bool read()override{QTimer::singleShot(0,this,[this]{emit readFinished({"synthetic-key",{}});});return true;}
    bool save(const QString&)override{return false;}bool remove()override{return false;}
    bool busy()const override{return false;}QString description()const override{return "fake";}
};
class HTTPSelection:public ISelectionProvider {
public:std::optional<Selection> currentSelection()override{return std::nullopt;}
};
class TestProviders:public QObject {
 Q_OBJECT
private slots:
 void contracts_data(){QTest::addColumn<QString>("id");for(auto id:{"openai","anthropic","gemini","openai_compatible"})QTest::newRow(id)<<QString(id);}
 void contracts(){
    QFETCH(QString,id);Server s;s.body=success(id);auto provider=make(id,s);QSignalSpy spy(provider.get(),&IAIProvider::completed);
    QVERIFY(provider->generate(request(id,s)));QVERIFY(provider->busy());QVERIFY(!provider->generate({}));
    QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).text,QString("visible answer"));
    QVERIFY(!provider->busy());const auto raw=s.captured;const auto header=raw.left(raw.indexOf("\r\n\r\n")).toLower();
    const auto body=QJsonDocument::fromJson(raw.mid(raw.indexOf("\r\n\r\n")+4)).object();
    QCOMPARE(body["model"].toString(),QString("configured-model"));QVERIFY(!raw.left(raw.indexOf("\r\n")).contains("synthetic-key"));
    QVERIFY(!body.contains("previous_interaction_id"));QVERIFY(!body.contains("tools"));
    if(id=="anthropic"){
        QVERIFY(raw.startsWith("POST /v1/messages "));QVERIFY(header.contains("x-api-key: synthetic-key"));QVERIFY(header.contains("anthropic-version: 2023-06-01"));
        QCOMPARE(body["system"].toString(),QString("system task"));QCOMPARE(body["max_tokens"].toInt(),1234);
        QCOMPARE(body["messages"].toArray()[0].toObject()["content"].toString(),QString("source text"));
    } else if(id=="gemini"){
        QVERIFY(raw.startsWith("POST /v1/interactions "));QVERIFY(header.contains("x-goog-api-key: synthetic-key"));
        QCOMPARE(body["system_instruction"].toString(),QString("system task"));QCOMPARE(body["input"].toString(),QString("source text"));
        QCOMPARE(body["generation_config"].toObject()["max_output_tokens"].toInt(),1234);QVERIFY(body.contains("store"));QVERIFY(!body["store"].toBool());
    } else {
        QVERIFY(raw.startsWith("POST /v1/responses "));QVERIFY(header.contains("authorization: bearer synthetic-key"));
        QCOMPARE(body["instructions"].toString(),QString("system task"));QCOMPARE(body["input"].toString(),QString("source text"));QVERIFY(body.contains("store"));QVERIFY(!body["store"].toBool());
    }
 }
 void failures_data(){contracts_data();}
 void failures(){
    QFETCH(QString,id);
    const QList<QPair<int,AIError>> cases{{401,AIError::Authentication},{403,AIError::Authentication},{429,AIError::RateLimited},{500,AIError::Server},{302,AIError::Server},{200,AIError::InvalidJson}};
    for(const auto& c:cases){Server s;s.status=c.first;s.body="PRIVATE RAW ERROR";auto p=make(id,s);QSignalSpy spy(p.get(),&IAIProvider::completed);QVERIFY(p->generate(request(id,s)));QTRY_COMPARE(spy.size(),1);auto r=qvariant_cast<AIResult>(spy[0][0]);QCOMPARE(r.error,c.second);QVERIFY(!r.message.contains("PRIVATE"));QVERIFY(r.text.isEmpty());}
    {Server s;s.body=id=="anthropic"?QByteArray(R"({"type":"message","role":"assistant","stop_reason":"end_turn","content":[]})"):id=="gemini"?QByteArray(R"({"status":"completed","steps":[]})"):QByteArray(R"({"status":"completed","output":[]})");
    auto p=make(id,s);QSignalSpy spy(p.get(),&IAIProvider::completed);QVERIFY(p->generate(request(id,s)));QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error,AIError::EmptyResponse);}
    {Server s;s.body=QByteArray(4*1024*1024+1,'x');auto p=make(id,s);QSignalSpy spy(p.get(),&IAIProvider::completed);QVERIFY(p->generate(request(id,s)));QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error,AIError::TooLarge);}
 }
 void geminiBlockedErrors_data(){
    QTest::addColumn<QString>("code");QTest::addColumn<bool>("refused");
    for(const auto* code:{"safety","jailbreak","recitation","language","prohibited_content","spii","blocklist",
        "image_safety","image_prohibited_content","image_recitation","image_other","content_blocked"})QTest::newRow(code)<<QString(code)<<true;
    QTest::newRow("unknown error")<<QString("unknown_error")<<false;
 }
 void geminiBlockedErrors(){
    QFETCH(QString,code);QFETCH(bool,refused);Server s;s.status=400;
    s.body=QJsonDocument(QJsonObject{{"error",QJsonObject{{"code",code},{"message","PRIVATE SOURCE AND KEY"}}}}).toJson(QJsonDocument::Compact);
    auto p=make("gemini",s);QSignalSpy spy(p.get(),&IAIProvider::completed);
    QVERIFY(p->generate(request("gemini",s)));QTRY_COMPARE(spy.size(),1);
    const auto result=qvariant_cast<AIResult>(spy[0][0]);
    QCOMPARE(result.error,refused?AIError::Refused:AIError::Server);QVERIFY(result.text.isEmpty());QVERIFY(!result.message.contains("PRIVATE"));
 }
 void anthropicStopReasons_data(){
    QTest::addColumn<QString>("reason");QTest::addColumn<bool>("refused");
    QTest::newRow("refusal")<<QString("refusal")<<true;
    QTest::newRow("max_tokens")<<QString("max_tokens")<<false;
 }
 void anthropicStopReasons(){
    QFETCH(QString,reason);QFETCH(bool,refused);Server s;
    s.body=QJsonDocument(QJsonObject{{"type","message"},{"role","assistant"},{"stop_reason",reason},
        {"content",QJsonArray{QJsonObject{{"type","text"},{"text","PRIVATE PARTIAL RESPONSE"}}}}}).toJson(QJsonDocument::Compact);
    auto p=make("anthropic",s);QSignalSpy spy(p.get(),&IAIProvider::completed);
    QVERIFY(p->generate(request("anthropic",s)));QTRY_COMPARE(spy.size(),1);
    const auto result=qvariant_cast<AIResult>(spy[0][0]);
    QCOMPARE(result.error,refused?AIError::Refused:AIError::Incomplete);QVERIFY(result.text.isEmpty());QVERIFY(!result.message.contains("PRIVATE"));
 }
 void cancelAndTimeout_data(){contracts_data();}
 void cancelAndTimeout(){
    QFETCH(QString,id);Server s;s.hang=true;auto p=make(id,s,50);QSignalSpy spy(p.get(),&IAIProvider::completed);
    QVERIFY(p->generate(request(id,s)));QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error,AIError::Timeout);
    QVERIFY(p->generate(request(id,s)));p->cancel();QTRY_COMPARE(spy.size(),2);QCOMPARE(qvariant_cast<AIResult>(spy[1][0]).error,AIError::Cancelled);QVERIFY(!p->busy());
    auto invalid=request(id,s);invalid.apiKey.clear();QVERIFY(p->generate(invalid));QVERIFY(p->busy());p->cancel();
    QTRY_COMPARE(spy.size(),3);QCOMPARE(qvariant_cast<AIResult>(spy[2][0]).error,AIError::Cancelled);
 }
 void compatibleRejectsUnsafeUrl(){
    OpenAICompatibleProvider p;QSignalSpy spy(&p,&IAIProvider::completed);
    for(const QString& url:{"http://example.com/v1","https://user:pass@example.com/v1","https://example.com/v1?key=private","https://example.com/v1#private","relative/path"}){
        QVERIFY(p.generate({{"s","u",{}},"model","synthetic",100,"openai_compatible",url}));
        QTRY_VERIFY(spy.size()>0);QCOMPARE(qvariant_cast<AIResult>(spy.takeFirst()[0]).error,AIError::Network);
    }
 }
 void serviceRoutesAndIsolatesFailure(){
    Server a,b;a.status=401;a.body="PRIVATE";b.body=success("openai");
    AIService service(nullptr,{{"anthropic",new AnthropicProvider(nullptr,QUrl(a.base().toString()+"/messages"))},{"openai",new OpenAIProvider(nullptr,QUrl(b.base().toString()+"/responses"))}});
    QSignalSpy spy(&service,&IAIProvider::completed);QVERIFY(service.generate(request("anthropic",a)));QVERIFY(!service.generate(request("openai",b)));
    QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<AIResult>(spy[0][0]).error,AIError::Authentication);
    QVERIFY(service.generate(request("openai",b)));QTRY_COMPARE(spy.size(),2);QCOMPARE(qvariant_cast<AIResult>(spy[1][0]).error,AIError::None);QCOMPARE(a.requests,1);QCOMPARE(b.requests,1);
 }
 void productPrivacyOverHTTP(){
    Server server;server.body=success("openai");
    OpenAIProvider ai(nullptr,QUrl(server.base().toString()+"/responses"));HTTPSecret secret;HTTPSelection selection;
    QTemporaryDir dir;QSettings settings(dir.filePath("settings.ini"),QSettings::IniFormat);
    Settings{"snapshot-model","English"}.save(settings);Profile profile;profile.role="PRIVATE_PROFILE_A";QVERIFY(profile.save(settings));
    settings.setValue("profile/enabled",true);AppController controller(ai,secret,selection,settings);QSignalSpy done(&controller,&AppController::finished);
    int count=0;
    for(auto feature:{Feature::PlainSpeak,Feature::Summarize,Feature::Polish,Feature::Relevance,Feature::AddInsight}) {
        QVERIFY(controller.runSelection(feature,{"SOURCE_A",{}, {}}));++count;QTRY_COMPARE(done.size(),count);
        QCOMPARE(server.captured.contains("PRIVATE_PROFILE_A"),feature==Feature::Relevance || feature==Feature::AddInsight);
    }
    settings.setValue("profile/enabled",false);QVERIFY(controller.runSelection(Feature::Relevance,{"SOURCE_A",{}, {}}));
    ++count;QTRY_COMPARE(done.size(),count);QVERIFY(!server.captured.contains("PRIVATE_PROFILE_A"));
    const int before=server.requests;QVERIFY(controller.runSelection(Feature::Reply,{"SOURCE_A",{}, {}}));QTest::qWait(20);QCOMPARE(server.requests,before);
    settings.setValue("profile/enabled",true);QVERIFY(controller.generateReply({"SOURCE_A",{}, {}},"Professional","PRIVATE_INTENT"));
    ++count;QTRY_COMPARE(done.size(),count);QVERIFY(server.captured.contains("PRIVATE_PROFILE_A"));QVERIFY(server.captured.contains("PRIVATE_INTENT"));
    profile.role="PRIVATE_PROFILE_B";QVERIFY(profile.save(settings));Settings{"changed-model","English"}.save(settings);
    QVERIFY(controller.refine("Shorter"));++count;QTRY_COMPARE(done.size(),count);
    QVERIFY(server.captured.contains("visible answer"));QVERIFY(server.captured.contains("SOURCE_A"));QVERIFY(server.captured.contains("snapshot-model"));
    QVERIFY(!server.captured.contains("PRIVATE_PROFILE_B"));
    QVERIFY(controller.refine("Regenerate"));++count;QTRY_COMPARE(done.size(),count);QVERIFY(!server.captured.contains("current_result"));
    QVERIFY(controller.testConnection());++count;QTRY_COMPARE(done.size(),count);
    QVERIFY(!server.captured.contains("PRIVATE_PROFILE"));QVERIFY(!server.captured.contains("SOURCE_A"));QVERIFY(!server.captured.contains("PRIVATE_INTENT"));
 }
};
QTEST_GUILESS_MAIN(TestProviders)
#include "TestProviders.moc"
