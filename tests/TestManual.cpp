#include <QtTest>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include "core/SelectionResolver.h"
#include "ui/ManualActionPalette.h"
using namespace ws;
class TestManual : public QObject {
    Q_OBJECT
private slots:
    void priorityAndBounds() {
        std::optional<Selection> a=Selection{"AT",QRect(1,2,3,4),"app"};
        QString p="PRIMARY", c="CLIPBOARD"; int pr=0,cr=0;
        SelectionResolver r([&]{return a;},[&]{++pr;return std::optional<Selection>{{p,{},"X11 PRIMARY"}};},
            [&]{++cr;return std::optional<Selection>{{c,{},"Clipboard"}};});
        auto v=r.resolve(); QVERIFY(v); QCOMPARE(v->source,SelectionSource::AtSpi);
        QCOMPARE(v->selection.sourceApplication,QString("app")); QCOMPARE(v->selection.anchorRect,a->anchorRect);
        QCOMPARE(pr,0); QCOMPARE(cr,0);
        a.reset(); v=r.resolve(); QVERIFY(v); QCOMPARE(v->source,SelectionSource::PrimarySelection);
        QCOMPARE(cr,0); p=" \n"; v=r.resolve(); QVERIFY(v); QCOMPARE(v->source,SelectionSource::Clipboard);
        p=QString(100001,'x'); c.clear(); QVERIFY(!r.resolve());
        p.clear(); c=QString(100001,'y'); QVERIFY(!r.resolve());
        c=QString(100000,'z'); QVERIFY(r.resolve()); c.clear(); QVERIFY(!r.resolve());
    }
    void snapshotPreviewCloseAndReplace() {
        ManualActionPalette palette; int calls=0; Selection chosen;
        connect(&palette,&ManualActionPalette::featureChosen,&palette,[&](Feature f,const Selection& s){
            QCOMPARE(f,Feature::Polish); ++calls; chosen=s;
        });
        ResolvedSelection v{{QString(2000,'a'),{},"Clipboard"},SelectionSource::Clipboard};
        palette.showSelection(v); QVERIFY(palette.isVisible()); QVERIFY(palette.currentSelection());
        QVERIFY(palette.findChild<QLabel*>("manualSource")->text().contains("Clipboard"));
        QVERIFY(palette.findChild<QPlainTextEdit*>("manualPreview")->toPlainText().size()<=1050);
        v.selection.text="external mutation"; QCOMPARE(palette.currentSelection()->selection.text.size(),2000);
        palette.showSelection({{"B",{},"X11 PRIMARY"},SelectionSource::PrimarySelection});
        QCOMPARE(calls,0);
        palette.findChild<QPushButton*>("manualAction2")->click();
        QCOMPARE(calls,1); QCOMPARE(chosen.text,QString("B")); QVERIFY(!palette.currentSelection());
        palette.showSelection({{"stale",{},"Clipboard"},SelectionSource::Clipboard});
        palette.showMessage("No selected text found."); QVERIFY(!palette.currentSelection());
        palette.findChild<QPushButton*>("manualAction2")->click(); QCOMPARE(calls,1);
        palette.showSelection({{"clear on close",{},"Clipboard"},SelectionSource::Clipboard});
        palette.close(); QVERIFY(!palette.currentSelection());
        QVERIFY(palette.findChild<QPlainTextEdit*>("manualPreview")->toPlainText().isEmpty());
    }
};
QTEST_MAIN(TestManual)
#include "TestManual.moc"
