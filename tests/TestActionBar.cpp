#include <QtTest>
#include <QPushButton>
#include "ui/ActionBar.h"
#include "ui/ActionBarPlacement.h"
using namespace ws;
class TestActionBar : public QObject {
    Q_OBJECT
private slots:
    void placement_data() {
        QTest::addColumn<QRect>("anchor"); QTest::addColumn<QRect>("screen");
        QTest::addColumn<QPoint>("position"); QTest::addColumn<QString>("mode");
        const QRect s(0,0,800,600);
        QTest::newRow("above") << QRect(300,200,100,20) << s << QPoint(250,162) << "ABOVE_SELECTION";
        QTest::newRow("below") << QRect(300,10,100,20) << s << QPoint(250,38) << "BELOW_SELECTION";
        QTest::newRow("left") << QRect(0,200,20,20) << s << QPoint(0,162) << "ABOVE_SELECTION";
        QTest::newRow("right") << QRect(790,200,10,20) << s << QPoint(600,162) << "ABOVE_SELECTION";
        QTest::newRow("top") << QRect(300,-100,100,20) << s << QPoint(250,0) << "BELOW_SELECTION_CLAMPED";
        QTest::newRow("bottom") << QRect(300,700,100,20) << s << QPoint(250,570) << "ABOVE_SELECTION_CLAMPED";
        QTest::newRow("negative-monitor") << QRect(-700,200,100,20) << QRect(-800,0,800,600)
            << QPoint(-750,162) << "ABOVE_SELECTION";
    }
    void placement() {
        QFETCH(QRect,anchor); QFETCH(QRect,screen); QFETCH(QPoint,position); QFETCH(QString,mode);
        auto p=placeActionBar(anchor,{200,30},screen);
        QVERIFY(p); QCOMPARE(p->geometry.topLeft(),position);
        QVERIFY(screen.contains(p->geometry));
        // Edge clamping is explicitly represented in diagnostic metadata.
        QVERIFY(p->mode.startsWith(mode.split("_CLAMPED").first()));
    }
    void invalidPlacement() {
        QVERIFY(!placeActionBar({}, {200,30}, {0,0,800,600}));
        QVERIFY(!placeActionBar({1,1,20,20}, {900,30}, {0,0,800,600}));
        QVERIFY(!placeActionBar({1,1,20,20}, {200,30}, {}));
    }
    void snapshotsAndClicks() {
        ActionBar bar; int calls=0; Selection chosen; Feature action=Feature::PlainSpeak;
        connect(&bar,&ActionBar::featureChosen,&bar,[&](Feature f,const Selection& s) { ++calls; action=f; chosen=s; });
        Selection a{"A",QRect(50,100,80,20),"fixture"};
        bar.showForSelection(a,{50,50}); QVERIFY(bar.isVisible()); QVERIFY(bar.currentSelection());
        a.text="mutated outside";
        QCOMPARE(bar.currentSelection()->text,QString("A"));
        bar.showForSelection({"B",QRect(100,100,80,20),"fixture"},{100,50});
        QCOMPARE(bar.currentSelection()->text,QString("B"));
        QCOMPARE(calls,0);
        QTest::mouseClick(bar.findChild<QPushButton*>("action2"),Qt::LeftButton);
        QCOMPARE(calls,1); QCOMPARE(action,Feature::Polish); QCOMPARE(chosen.text,QString("B"));
        QVERIFY(!bar.isVisible()); QVERIFY(!bar.currentSelection());
        bar.findChild<QPushButton*>("action0")->click(); QCOMPARE(calls,1);
    }
    void dismissAndClear() {
        ActionBar bar; QSignalSpy dismissed(&bar,&ActionBar::dismissed);
        bar.showForSelection({"A",{},"fixture"},{20,20});
        bar.findChild<QPushButton*>("dismissActionBar")->click();
        QVERIFY(!bar.isVisible()); QVERIFY(!bar.currentSelection()); QCOMPARE(dismissed.size(),1);
        bar.showForSelection({"B",{},"fixture"},{20,20}); bar.hideBar();
        QVERIFY(!bar.currentSelection());
        bar.showForSelection({"  ",{},"fixture"},{20,20}); QVERIFY(!bar.isVisible());
    }
};
QTEST_MAIN(TestActionBar)
#include "TestActionBar.moc"
