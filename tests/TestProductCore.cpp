#include <QtTest>
#include <QTemporaryDir>
#include <QSet>
#include "core/Core.h"
#include "core/Profile.h"
#include "core/FeaturePreferences.h"
using namespace ws;
class TestProductCore : public QObject {
 Q_OBJECT
private slots:
 void contextBounds() {
    PromptRequest r{Feature::Reply,"source","Same as input",QString(8000,'p')};
    r.userIntent=QString(2000,'i');r.previousResult=QString(4*1024*1024,'r');r.refinementInstruction="Shorter";
    QVERIFY(PromptBuilder::build(r).error.isEmpty());
    r.profile+='p';QVERIFY(!PromptBuilder::build(r).error.isEmpty());r.profile.chop(1);
    r.userIntent+='i';QVERIFY(!PromptBuilder::build(r).error.isEmpty());r.userIntent.chop(1);
    r.previousResult+='r';QVERIFY(!PromptBuilder::build(r).error.isEmpty());r.previousResult.clear();
    QVERIFY(!PromptBuilder::build(r).error.isEmpty());
    for(const auto& f:FeatureRegistry::all())QVERIFY(PromptBuilder::build({f.id,"source","Same as input",{}}).system.contains("same language"));
 }
 void allProfileLimits() {
    const QList<QPair<QString Profile::*,int>> fields{{&Profile::displayName,100},{&Profile::role,200},{&Profile::team,200},
        {&Profile::responsibilities,2000},{&Profile::currentProjects,2000},{&Profile::communicationPreferences,1000},{&Profile::additionalContext,2000}};
    Profile total;
    for(const auto& f:fields) {Profile p;p.*f.first=QString(f.second,'x');QVERIFY(p.valid());(p.*f.first)+='x';QVERIFY(!p.valid());total.*f.first=QString(f.second,'x');}
    QVERIFY(total.valid());QVERIFY(total.serialize().size()<=8000);QVERIFY(!total.serialize().isEmpty());
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);s.setValue("profile/team",QString(201,'x'));
    QVERIFY(Profile::load(s).serialize().isEmpty());
 }
 void migrationDoesNotOverwriteNewConfig() {
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);
    s.setValue("ai/model","legacy");s.setValue("ai/openai/model","new");s.setValue("output/language","日本語");
    QCOMPARE(Settings::load(s).model,QString("new"));QCOMPARE(Settings::load(s).outputLanguage,QString("日本語"));
    QCOMPARE(s.value("ai/model").toString(),QString("legacy"));QCOMPARE(s.value("ai/openai/model").toString(),QString("new"));
 }
 void registry() {
    auto all=FeatureRegistry::all();QCOMPARE(all.size(),6);
    QSet<QString> ids;
    for(const auto& f:all) {QVERIFY(!f.stableId.isEmpty());QVERIFY(!ids.contains(f.stableId));ids.insert(f.stableId);QVERIFY(!f.name.isEmpty());QVERIFY(!f.description.isEmpty());QCOMPARE(FeatureRegistry::fromId(f.stableId).value(),f.id);}
    QVERIFY(FeatureRegistry::info(Feature::Reply).requiresComposer);
    for(auto f:{Feature::Reply,Feature::Relevance,Feature::AddInsight})QVERIFY(FeatureRegistry::info(f).requiresProfile);
    for(auto f:{Feature::PlainSpeak,Feature::Summarize,Feature::Polish})QVERIFY(!FeatureRegistry::info(f).requiresProfile);
    QVERIFY(!FeatureRegistry::fromId("unknown"));
 }
 void prompts() {
    for(auto f:{Feature::PlainSpeak,Feature::Summarize,Feature::Polish,Feature::Reply,Feature::Relevance,Feature::AddInsight}) {
        PromptRequest r{f,"facts 42 Friday","English","PRIVATE PROFILE"};
        auto p=PromptBuilder::build(r);QVERIFY2(p.error.isEmpty(),qPrintable(p.error));
        QVERIFY(p.system.contains("facts"));QVERIFY(p.system.contains("English"));QVERIFY(p.system.contains("source"));
        QCOMPARE(p.user.contains("PRIVATE PROFILE"),FeatureRegistry::info(f).requiresProfile);
        r.selectedText="  ";QVERIFY(!PromptBuilder::build(r).error.isEmpty());
        r.selectedText=QString(100001,'x');QVERIFY(!PromptBuilder::build(r).error.isEmpty());
    }
    PromptRequest r{Feature::Reply,"Can you finish Friday?","Same as input","role"};
    r.variant="Professional";r.userIntent="I can deliver Monday";
    auto p=PromptBuilder::build(r);QVERIFY(p.system.contains("Professional"));QVERIFY(p.system.contains("same language"));
    QVERIFY(p.user.contains("Can you finish Friday?"));QVERIFY(p.user.contains("I can deliver Monday"));
    r.previousResult="CURRENT RESULT";r.refinementInstruction="Shorter";p=PromptBuilder::build(r);
    QVERIFY(p.user.contains("CURRENT RESULT"));QVERIFY(p.system.contains("Shorter"));
    r.refinementInstruction="unknown";QVERIFY(!PromptBuilder::build(r).error.isEmpty());
    r.refinementInstruction.clear();r.variant="unknown";QVERIFY(!PromptBuilder::build(r).error.isEmpty());
 }
 void profileBoundsAndPersistence() {
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);Profile p;
    p.role="ROLE";p.responsibilities="RESPONSIBILITY";QVERIFY(p.valid());QVERIFY(p.save(s));
    QCOMPARE(Profile::load(s).role,p.role);QVERIFY(p.serialize().contains("ROLE"));
    p.displayName=QString(101,'x');QVERIFY(!p.valid());QVERIFY(!p.save(s));
    QCOMPARE(Profile::load(s).role,QString("ROLE"));
    p.displayName.clear();p.role=QString(201,'x');QVERIFY(!p.valid());
    p.role.clear();p.responsibilities=QString(2001,'x');QVERIFY(!p.valid());
    Profile::clear(s);QVERIFY(Profile::load(s).serialize().isEmpty());QVERIFY(!s.value("profile/enabled",false).toBool());
 }
 void preferences() {
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);
    auto p=FeaturePreferences::load(s);QCOMPARE(p.enabled.size(),6);QVERIFY(p.quickActions.size()>=3);QVERIFY(p.quickActions.size()<=5);
    s.setValue("features/enabled",QStringList{"reply","unknown","reply","polish"});
    s.setValue("features/quickActions",QStringList{"unknown","relevance","reply"});
    p=FeaturePreferences::load(s);QCOMPARE(p.enabled,QStringList({"reply","polish"}));
    QCOMPARE(p.quickActions,QStringList({"reply","polish"}));QVERIFY(p.save(s));
    s.setValue("features/enabled",QStringList{});p=FeaturePreferences::load(s);QVERIFY(p.enabled.isEmpty());QVERIFY(p.quickActions.isEmpty());
 }
 void providerMigration() {
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);
    s.setValue("ai/model","legacy");s.setValue("ui/automaticPopup",true);s.setValue("shortcut/enabled",true);
    auto initial=Settings::load(s);QCOMPARE(initial.model,QString("legacy"));QCOMPARE(initial.providerId,QString("openai"));
    QCOMPARE(s.value("ai/openai/model").toString(),QString("legacy"));
    auto keys=s.allKeys();Settings::load(s);QCOMPARE(s.allKeys(),keys);
    Settings other{"anthropic-model","中文","anthropic"};QVERIFY(other.save(s));
    QCOMPARE(Settings::load(s).model,QString("anthropic-model"));
    QCOMPARE(Settings::forProvider(s,"openai").model,QString("legacy"));
    QCOMPARE(s.value("ai/model").toString(),QString("legacy"));QVERIFY(s.value("ui/automaticPopup").toBool());QVERIFY(s.value("shortcut/enabled").toBool());
 }
};
QTEST_GUILESS_MAIN(TestProductCore)
#include "TestProductCore.moc"
