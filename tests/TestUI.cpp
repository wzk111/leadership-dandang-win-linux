#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QPushButton>
#include <QPlainTextEdit>
#include "ui/ResultCard.h"
#include "platform/ClipboardSelectionProvider.h"
using namespace ws;
class TestUI : public QObject {
    Q_OBJECT
private slots:
    void explicitClipboard() {
        auto* cb = QApplication::clipboard(); cb->setText("test copied text");
        ClipboardSelectionProvider p(*cb);
        const auto selection = p.currentSelection(); QVERIFY(selection.has_value());
        QCOMPARE(selection->text, QString("test copied text"));
        QCOMPARE(cb->text(), QString("test copied text"));
        cb->setText("  "); QVERIFY(!p.currentSelection().has_value());
        cb->clear(); QVERIFY(!p.currentSelection().has_value());
    }
    void resultLifecycle() {
        ResultCard card;
        auto* copy = card.findChild<QPushButton*>("copyResult");
        auto* cancel = card.findChild<QPushButton*>("cancelRequest");
        auto* output = card.findChild<QPlainTextEdit*>("resultText");
        QVERIFY(copy); QVERIFY(cancel); QVERIFY(output);
        card.loading(); QVERIFY(!copy->isEnabled()); QVERIFY(cancel->isEnabled());
        card.success("<script>plain text only</script>");
        QCOMPARE(output->toPlainText(), QString("<script>plain text only</script>"));
        QVERIFY(copy->isEnabled()); copy->click();
        QCOMPARE(QApplication::clipboard()->text(), output->toPlainText());
        card.loading(); QVERIFY(output->toPlainText().isEmpty());
        card.error("Network unavailable"); QVERIFY(!copy->isEnabled());
        card.loading(); QSignalSpy spy(&card, &ResultCard::cancelRequested); card.close();
        QCOMPARE(spy.size(), 1);
    }
};
QTEST_MAIN(TestUI)
#include "TestUI.moc"
