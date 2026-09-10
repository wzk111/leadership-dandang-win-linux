#include <QtTest>
#include <QTemporaryDir>
#include "app/AppController.h"
using namespace ws;
class FakeAI : public IAIProvider {
public:
    AIRequest last; int calls = 0; bool active = false;
    bool generate(const AIRequest& r) override { last = r; ++calls; active = true; return true; }
    void cancel() override { active = false; emit completed({{}, AIError::Cancelled, "Cancelled"}); }
    bool busy() const override { return active; }
    void respond() { active = false; emit completed({"synthetic result", AIError::None, {}}); }
};
class FakeSecret : public ISecretStore {
public:
    int reads = 0; bool active = false;
    bool read() override { ++reads; active = true; return true; }
    bool save(const QString&) override { return false; }
    bool remove() override { return false; }
    bool busy() const override { return active; }
    QString description() const override { return "mock"; }
    void respond(QString key = "test-key") { active = false; emit readFinished({key, {}}); }
};
class FakeSelection : public ISelectionProvider {
public:
    QString text = "explicit text"; int reads = 0;
    std::optional<Selection> currentSelection() override {
        ++reads; if (text.isEmpty()) return std::nullopt;
        return Selection{text, {}, {}};
    }
};
class TestController : public QObject {
    Q_OBJECT
private slots:
    void explicitActionOnly() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        Settings{"configured-model", "English"}.save(settings);
        FakeAI ai; FakeSecret secret; FakeSelection selection;
        AppController c(ai, secret, selection, settings);
        QSignalSpy finished(&c, &AppController::finished);
        QCOMPARE(selection.reads, 0); QCOMPARE(ai.calls, 0); QCOMPARE(secret.reads, 0);
        QVERIFY(c.captureClipboard()); QCOMPARE(ai.calls, 0); QCOMPARE(secret.reads, 0);
        QVERIFY(c.run(Feature::Summarize)); QVERIFY(c.busy());
        QVERIFY(!c.run(Feature::Polish)); QVERIFY(!c.captureClipboard());
        QCOMPARE(secret.reads, 1); QCOMPARE(ai.calls, 0);
        selection.text = "changed clipboard";
        secret.respond(); QCOMPARE(ai.calls, 1);
        QCOMPARE(ai.last.prompt.user, QString("explicit text"));
        ai.respond(); QVERIFY(!c.busy()); QCOMPARE(finished.size(), 1);
    }
    void emptyClearsPreviousCapture() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        Settings{"model", "English"}.save(settings);
        FakeAI ai; FakeSecret secret; FakeSelection selection;
        AppController c(ai, secret, selection, settings);
        QVERIFY(c.captureClipboard()); selection.text.clear();
        QVERIFY(!c.captureClipboard()); QVERIFY(!c.run(Feature::Polish));
        QCOMPARE(secret.reads, 0); QCOMPARE(ai.calls, 0);
    }
    void busyKeyringExplainsFailure() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        Settings{"model", "English"}.save(settings);
        FakeAI ai; FakeSecret secret; FakeSelection selection;
        AppController c(ai, secret, selection, settings);
        QSignalSpy finished(&c, &AppController::finished);
        QVERIFY(c.captureClipboard()); secret.active = true;
        QVERIFY(!c.run(Feature::Polish));
        QCOMPARE(finished.size(), 1);
        QVERIFY(!qvariant_cast<AIResult>(finished[0][0]).message.isEmpty());
        QCOMPARE(ai.calls, 0);
    }
    void cancelDuringKeyRead() {
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        Settings{"model", "English"}.save(settings);
        FakeAI ai; FakeSecret secret; FakeSelection selection;
        AppController c(ai, secret, selection, settings);
        QVERIFY(c.captureClipboard()); QVERIFY(c.run(Feature::Polish));
        c.cancel(); secret.respond(); QCOMPARE(ai.calls, 0); QVERIFY(!c.busy());
    }
};
QTEST_GUILESS_MAIN(TestController)
#include "TestController.moc"
