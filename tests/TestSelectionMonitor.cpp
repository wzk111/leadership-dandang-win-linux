#include <QtTest>
#include "platform/linux/AtSpiSelectionMonitor.h"
using namespace ws;
class FakeSession : public IAtSpiSession {
public:
    int starts = 0, stops = 0, reads = 0;
    void start() override { ++starts; MonitorStatus s; s.available=s.initialized=s.running=true; s.description="Running"; emit state(s); }
    void stop() override { ++stops; }
    void extract(quint64 seq) override { ++reads; emit extracted(seq, {Selection{"hello", {}, "synthetic"}, "Text retrieved"}); }
};
class TestSelectionMonitor : public QObject {
    Q_OBJECT
private slots:
    void debounceAndIdempotence() {
        auto backend = std::make_unique<FakeSession>(); auto* b = backend.get();
        AtSpiSelectionMonitor monitor(std::move(backend));
        QSignalSpy detected(&monitor, &ISelectionMonitor::selectionDetected);
        QVERIFY(monitor.start()); QVERIFY(monitor.start()); QCOMPARE(b->starts, 1);
        emit b->eventReceived(1); emit b->eventReceived(2); emit b->eventReceived(3);
        QCOMPARE(b->reads, 0); QTRY_COMPARE(b->reads, 1); QCOMPARE(detected.size(), 1);
        emit b->eventReceived(4); QTRY_COMPARE(b->reads, 2); QCOMPARE(detected.size(), 1);
        monitor.stop(); monitor.stop(); QCOMPARE(b->stops, 1); QVERIFY(!monitor.latestSelection());
        emit b->extracted(4, {Selection{"late", {}, {}}, "late"}); QVERIFY(!monitor.latestSelection());
    }
    void emptyClearsAndPendingStop() {
        auto backend = std::make_unique<FakeSession>(); auto* b = backend.get();
        AtSpiSelectionMonitor monitor(std::move(backend));
        QVERIFY(monitor.start()); emit b->eventReceived(1); QTRY_VERIFY(monitor.latestSelection());
        QSignalSpy cleared(&monitor, &ISelectionMonitor::selectionCleared);
        emit b->extracted(1, {{}, "No selection"}); QVERIFY(!monitor.latestSelection()); QCOMPARE(cleared.size(), 1);
        emit b->eventReceived(2); monitor.stop(); QTest::qWait(120); QCOMPARE(b->reads, 1);
    }
};
QTEST_GUILESS_MAIN(TestSelectionMonitor)
#include "TestSelectionMonitor.moc"
