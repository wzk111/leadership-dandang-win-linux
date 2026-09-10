#include <QtTest>
#include <QProcess>
#include "platform/linux/AtSpiSelectionMonitor.h"
using namespace ws;
class TestAtSpiRuntime : public QObject {
    Q_OBJECT
private slots:
    void realEvents() {
        AtSpiSelectionMonitor monitor(createAtSpiSession());
        QSignalSpy detected(&monitor,&ISelectionMonitor::selectionDetected);
        QVERIFY(monitor.start());
        QTRY_VERIFY_WITH_TIMEOUT(monitor.status().running,10000);
        QProcess fixture;
        fixture.start(QCoreApplication::applicationDirPath()+"/accessible_fixture");
        QVERIFY(fixture.waitForStarted());
        QTRY_VERIFY_WITH_TIMEOUT(detected.size()>0,10000);
        const auto selection=qvariant_cast<Selection>(detected.last()[0]);
        QCOMPARE(selection.text,QString("synthetic selection"));
        QVERIFY(!selection.sourceApplication.isEmpty());
        // Qt QTextEdit exposes usable geometry under this synthetic X11 session.
        QVERIFY(selection.anchorRect.has_value());
        QVERIFY(selection.anchorRect->width()>0); QVERIFY(selection.anchorRect->height()>0);
        QSignalSpy cleared(&monitor,&ISelectionMonitor::selectionCleared);
        QTRY_VERIFY_WITH_TIMEOUT(cleared.size()>0,5000);
        monitor.stop(); QVERIFY(!monitor.latestSelection()); QVERIFY(!monitor.status().running);
        fixture.terminate(); if(!fixture.waitForFinished(3000)) { fixture.kill(); fixture.waitForFinished(); }
    }
};
int main(int argc,char** argv) {
    qputenv("QT_NO_GLIB","1");
    QCoreApplication app(argc,argv);
    TestAtSpiRuntime test;
    return QTest::qExec(&test,argc,argv);
}
#include "TestAtSpiRuntime.moc"
