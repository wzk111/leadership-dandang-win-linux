#include <QtTest>
#include <QTemporaryDir>
#include "app/AppController.h"
#include "core/Profile.h"
using namespace ws;
class ProductAI : public IAIProvider {
public:
    AIRequest request; int calls=0; bool active=false;
    bool generate(const AIRequest& r) override { if(active)return false; request=r;++calls;active=true;return true; }
    bool busy() const override {return active;}
    void cancel() override {if(active)finish({},AIError::Cancelled);}
    void finish(QString text="answer",AIError error=AIError::None) {active=false;emit completed({text,error,error==AIError::None?QString{}:QString("Safe error")});}
};
class ProductSecret : public ISecretStore {
public:
    int reads=0; bool active=false;
    bool read() override {if(active)return false;++reads;active=true;return true;}
    bool save(const QString&) override {return false;} bool remove() override {return false;}
    bool busy() const override {return active;} QString description() const override {return "fixture";}
    void finish() {active=false;emit readFinished({"synthetic-key",{}});}
};
class ProductSelection : public ISelectionProvider {
public:
    int reads=0; QString value="source A";
    std::optional<Selection> currentSelection() override {++reads;return Selection{value,{},"fixture"};}
};
class TestGeneration : public QObject {
    Q_OBJECT
private slots:
    void replyConsentAndImmutableSource() {
        QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
        ProductAI ai;ProductSecret secret;ProductSelection selection;AppController c(ai,secret,selection,s);
        Selection composer;int opens=0;
        connect(&c,&AppController::replyRequested,&c,[&](const Selection& value){composer=value;++opens;});
        QVERIFY(c.captureClipboard());QVERIFY(c.run(Feature::Reply));QCOMPARE(opens,1);
        QCOMPARE(secret.reads,0);QCOMPARE(ai.calls,0);
        selection.value="source B";QVERIFY(c.captureClipboard());QCOMPARE(composer.text,QString("source A"));
        QVERIFY(c.generateReply(composer,"Professional","deliver Monday"));
        QCOMPARE(secret.reads,1);secret.finish();QCOMPARE(ai.calls,1);
        QVERIFY(ai.request.prompt.user.contains("source A"));QVERIFY(ai.request.prompt.user.contains("deliver Monday"));
        QVERIFY(!ai.request.prompt.user.contains("source B"));QVERIFY(ai.request.prompt.system.contains("Professional"));
        ai.finish();QVERIFY(c.generationContext());QCOMPARE(c.generationContext()->lastResult,QString("answer"));
    }
    void profileSnapshotAndRefinementChain() {
        QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model-A","English"}.save(s);
        Profile p;p.role="PROFILE A";QVERIFY(p.save(s));s.setValue("profile/enabled",true);
        ProductAI ai;ProductSecret secret;ProductSelection selection;AppController c(ai,secret,selection,s);
        QVERIFY(c.runSelection(Feature::PlainSpeak,{"source A",{},"fixture"}));secret.finish();
        QVERIFY(!ai.request.prompt.user.contains("PROFILE A"));ai.finish();
        QVERIFY(c.runSelection(Feature::Relevance,{"source A",{},"fixture"}));secret.finish();
        QVERIFY(ai.request.prompt.user.contains("PROFILE A"));ai.finish("result A");
        p.role="PROFILE B";QVERIFY(p.save(s));Settings{"model-B","中文","anthropic"}.save(s);
        selection.value="source B";QVERIFY(c.captureClipboard());
        QVERIFY(c.refine("Regenerate"));secret.finish();
        QCOMPARE(ai.request.model,QString("model-A"));
        QVERIFY(ai.request.prompt.user.contains("PROFILE A"));QVERIFY(!ai.request.prompt.user.contains("PROFILE B"));
        QVERIFY(!ai.request.prompt.user.contains("result A"));ai.finish("result regenerated");
        QVERIFY(c.refine("Shorter"));secret.finish();
        QVERIFY(ai.request.prompt.user.contains("result regenerated"));QVERIFY(ai.request.prompt.user.contains("source A"));
        QVERIFY(!ai.request.prompt.user.contains("source B"));QVERIFY(!c.refine("Longer"));
        ai.finish("short result");QCOMPARE(c.generationContext()->lastResult,QString("short result"));
        QVERIFY(c.refine("Friendlier"));secret.finish();ai.finish({},AIError::RateLimited);
        QCOMPARE(c.generationContext()->lastResult,QString("short result"));
        const int reads=selection.reads;
        QVERIFY(c.refine("More Direct"));secret.finish();c.cancel();
        QCOMPARE(c.generationContext()->lastResult,QString("short result"));QCOMPARE(selection.reads,reads);
        c.clearGeneration();QVERIFY(!c.generationContext());QVERIFY(!c.refine("Shorter"));
        QVERIFY(c.runSelection(Feature::Relevance,{"fresh source",{},"fixture"}));secret.finish();
        QVERIFY(ai.request.prompt.user.contains("PROFILE B"));QCOMPARE(ai.request.model,QString("model-B"));ai.finish();
        s.setValue("profile/enabled",false);
        QVERIFY(c.runSelection(Feature::Relevance,{"fresh source",{},"fixture"}));secret.finish();
        QVERIFY(!ai.request.prompt.user.contains("PROFILE B"));ai.finish();
    }
    void closeDuringSecretReadDiscardsLateCompletion() {
        QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
        ProductAI ai;ProductSecret secret;ProductSelection selection;AppController c(ai,secret,selection,s);
        QSignalSpy finished(&c,&AppController::finished);
        QVERIFY(c.runSelection(Feature::Polish,{"draft",{}, {}}));c.clearGeneration();secret.finish();
        QCOMPARE(ai.calls,0);QVERIFY(!c.generationContext());QVERIFY(!c.busy());QCOMPARE(finished.size(),0);
    }
    void syntheticTestNeverReadsProfileOrSelection() {
        QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
        Profile p;p.role="PRIVATE PROFILE";QVERIFY(p.save(s));s.setValue("profile/enabled",true);
        ProductAI ai;ProductSecret secret;ProductSelection selection;AppController c(ai,secret,selection,s);
        QVERIFY(c.testConnection());secret.finish();QCOMPARE(selection.reads,0);
        QVERIFY(!ai.request.prompt.user.contains("PRIVATE PROFILE"));QVERIFY(ai.request.prompt.user.contains("OK"));
        ai.finish();QVERIFY(!c.generationContext());
    }
};
QTEST_GUILESS_MAIN(TestGeneration)
#include "TestGeneration.moc"
