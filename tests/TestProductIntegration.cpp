#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include "app/Application.h"
using namespace ws;
class IntegrationAI:public IAIProvider {
public:
    int calls=0;bool active=false;
    bool generate(const AIRequest&) override {++calls;active=true;return true;}
    void cancel() override {if(active){active=false;emit completed({{},AIError::Cancelled,"Cancelled"});}}
    bool busy()const override{return active;}
};
class IntegrationSecret:public ISecretStore {
public:
    int reads=0;bool active=false;
    bool read()override{++reads;active=true;return true;}bool save(const QString&)override{return false;}bool remove()override{return false;}
    bool busy()const override{return active;}QString description()const override{return "fake";}
    void finish(){active=false;emit readFinished({"synthetic",{}});}
};
class IntegrationSelection:public ISelectionProvider {
public:
    std::optional<Selection> currentSelection()override{return Selection{"source",{},"fixture"};}
};
template<class T>T* window(){for(auto* w:QApplication::topLevelWidgets())if(auto* p=qobject_cast<T*>(w))return p;return nullptr;}
class TestProductIntegration:public QObject {
 Q_OBJECT
private slots:
 void closeRestoresWorkspace_data(){QTest::addColumn<bool>("duringAI");QTest::newRow("secret")<<false;QTest::newRow("AI")<<true;}
 void closeRestoresWorkspace(){
    QFETCH(bool,duringAI);QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
    IntegrationAI ai;IntegrationSecret secret;IntegrationSelection selection;AppController c(ai,secret,selection,s);Application app(c,secret,s);app.start();QApplication::setQuitOnLastWindowClosed(false);
    auto* workspace=window<WorkspaceWindow>();auto* result=window<ResultCard>();QVERIFY(workspace);QVERIFY(result);
    workspace->findChild<QPushButton*>("processClipboard")->click();workspace->findChild<QPushButton*>("feature0")->click();
    QVERIFY(!workspace->findChild<QPushButton*>("processClipboard")->isEnabled());
    if(duringAI)secret.finish();result->close();
    QVERIFY(workspace->findChild<QPushButton*>("processClipboard")->isEnabled());QVERIFY(!result->isVisible());QVERIFY(!c.generationContext());
    if(!duringAI)secret.finish();QVERIFY(!result->isVisible());
 }
 void profileProviderAndReplyAreLocal(){
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
    IntegrationAI ai;IntegrationSecret secret;IntegrationSelection selection;AppController c(ai,secret,selection,s);Application app(c,secret,s);app.start();QApplication::setQuitOnLastWindowClosed(false);
    auto* sw=window<SettingsWindow>();auto* pw=window<ProfileWindow>();QVERIFY(sw);QVERIFY(pw);
    auto* provider=sw->findChild<QComboBox*>("provider");QVERIFY(provider);provider->setCurrentIndex(1);
    pw->findChild<QPlainTextEdit*>("profile_role")->setPlainText("private profile");
    pw->findChild<QPushButton*>("saveProfile")->click();QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);
    QVERIFY(c.runSelection(Feature::Reply,{"source A",{}, {}}));auto* reply=window<ReplyComposer>();QVERIFY(reply);QVERIFY(reply->isVisible());
    QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);reply->close();QCOMPARE(ai.calls,0);
 }
 void customization(){
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Settings{"model","English"}.save(s);
    IntegrationAI ai;IntegrationSecret secret;IntegrationSelection selection;AppController c(ai,secret,selection,s);Application app(c,secret,s);
    auto* sw=window<SettingsWindow>();auto* bar=window<ActionBar>();auto* manual=window<ManualActionPalette>();QVERIFY(sw);QVERIFY(bar);QVERIFY(manual);
    auto* enabled=sw->findChild<QCheckBox*>("enabled_add_insight");QVERIFY(enabled);enabled->setChecked(false);
    sw->findChild<QPushButton*>("saveSettings")->click();
    QVERIFY(manual->findChild<QPushButton*>("manualAction5")->isHidden());
    int visible=0;for(int i=0;i<6;++i)if(!bar->findChild<QPushButton*>(QString("action%1").arg(i))->isHidden())++visible;
    QVERIFY(visible>=3 && visible<=5);QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);
 }
};
QTEST_MAIN(TestProductIntegration)
#include "TestProductIntegration.moc"
