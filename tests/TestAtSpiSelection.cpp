#include <QtTest>
#include "platform/linux/AtSpiSelectionProvider.h"
using namespace ws;
class TextSource : public IAtSpiTextSource {
public:
    int count = 1; int textCalls = 0;
    QList<QPair<int,int>> ranges{{0, 5}};
    std::optional<QString> value = QString("  hi ");
    std::optional<QRect> rect = QRect(-200, 20, 80, 15);
    int selectionCount() override { return count; }
    std::optional<QPair<int,int>> range(int i) override { return ranges.value(i); }
    std::optional<QString> text(int, int) override { ++textCalls; return value; }
    std::optional<QRect> rectangle(int, int) override { return rect; }
    QString application() override { return "Synthetic app"; }
};
class TestAtSpiSelection : public QObject {
    Q_OBJECT
private slots:
    void validPreservesText() {
        TextSource s; const auto r = AtSpiSelectionProvider::read(s);
        QVERIFY(r.selection); QCOMPARE(r.selection->text, QString("  hi "));
        QCOMPARE(r.selection->sourceApplication, QString("Synthetic app"));
        QCOMPARE(r.selection->anchorRect, s.rect);
    }
    void emptyAndError() {
        TextSource s; s.count = 0; QVERIFY(!AtSpiSelectionProvider::read(s).selection);
        s.count = -1; QVERIFY(!AtSpiSelectionProvider::read(s).description.isEmpty());
        s.count = 1; s.value.reset(); QVERIFY(!AtSpiSelectionProvider::read(s).selection);
        s.value = " \t"; QVERIFY(!AtSpiSelectionProvider::read(s).selection);
    }
    void multipleFirstValid() {
        TextSource s; s.count = 3; s.ranges = {{0,0},{2,7},{7,12}};
        QVERIFY(AtSpiSelectionProvider::read(s).selection); QCOMPARE(s.textCalls, 1);
    }
    void geometryOptional_data() {
        QTest::addColumn<QRect>("rect");
        QTest::newRow("negative") << QRect(1,1,-4,5);
        QTest::newRow("zero") << QRect(1,1,0,5);
        QTest::newRow("extreme") << QRect(2000000000,1,5,5);
    }
    void geometryOptional() {
        QFETCH(QRect, rect); TextSource s; s.rect = rect;
        const auto r = AtSpiSelectionProvider::read(s); QVERIFY(r.selection); QVERIFY(!r.selection->anchorRect);
        s.rect.reset(); QVERIFY(AtSpiSelectionProvider::read(s).selection);
    }
    void oversizedNotFetched() {
        TextSource s; s.ranges = {{0,100001}};
        QVERIFY(!AtSpiSelectionProvider::read(s).selection); QCOMPARE(s.textCalls, 0);
    }
};
QTEST_GUILESS_MAIN(TestAtSpiSelection)
#include "TestAtSpiSelection.moc"
