#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "core/Core.h"
using namespace ws;
class TestCore : public QObject {
    Q_OBJECT
private slots:
    void features() {
        const auto list = FeatureRegistry::all();
        QCOMPARE(list.size(), 3);
        QCOMPARE(list[0].name, QString("Plain Speak"));
        QCOMPARE(list[1].id, Feature::Summarize);
        QCOMPARE(list[2].id, Feature::Polish);
        for (const auto& f : list) { QVERIFY(!f.requiresProfile); QVERIFY(!f.description.isEmpty()); }
    }
    void promptIsolation() {
        for (const auto& f : {Feature::PlainSpeak, Feature::Summarize, Feature::Polish}) {
            const auto p = PromptBuilder::build({f, "Deadline 12:00, budget 42", "中文", "PRIVATE PROFILE"});
            QVERIFY(p.error.isEmpty());
            QCOMPARE(p.user, QString("Deadline 12:00, budget 42"));
            QVERIFY(p.system.contains("中文"));
            QVERIFY(!p.system.contains("PRIVATE PROFILE"));
            QVERIFY(!p.user.contains("PRIVATE PROFILE"));
            QVERIFY(p.system.contains("facts"));
        }
    }
    void invalidInput() {
        QVERIFY(!PromptBuilder::build({Feature::Polish, " \n\t", "", ""}).error.isEmpty());
        QVERIFY(!PromptBuilder::build({static_cast<Feature>(99), "hello", "", ""}).error.isEmpty());
        QVERIFY(!PromptBuilder::build({Feature::Polish, QString(100001, 'x'), "", ""}).error.isEmpty());
    }
    void languageDefault() {
        const auto p = PromptBuilder::build({Feature::Summarize, "hello", "", ""});
        QVERIFY(p.system.contains("same language"));
    }
    void settingsRoundTrip() {
        QTemporaryDir dir;
        QSettings store(dir.filePath("settings.ini"), QSettings::IniFormat);
        Settings s; s.model = "chosen-model"; s.outputLanguage = "中文";
        QVERIFY(s.save(store));
        const auto restored = Settings::load(store);
        QCOMPARE(restored.model, s.model);
        QCOMPARE(restored.outputLanguage, s.outputLanguage);
        QCOMPARE(store.allKeys().size(), 2);
        QVERIFY(!store.contains("apiKey"));
    }
};
QTEST_GUILESS_MAIN(TestCore)
#include "TestCore.moc"
