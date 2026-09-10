#include <QtTest>
#include <QUuid>
#include "platform/linux/LinuxSecretStore.h"
using namespace ws;
class TestSecret : public QObject {
    Q_OBJECT
private slots:
    void roundTrip() {
        LinuxSecretStore store(nullptr, "test-" + QUuid::createUuid().toString());
        QSignalSpy written(&store, &ISecretStore::writeFinished);
        QSignalSpy read(&store, &ISecretStore::readFinished);
        QVERIFY(store.save("synthetic-test-key"));
        QVERIFY(store.busy()); QVERIFY(!store.read());
        QTRY_COMPARE_WITH_TIMEOUT(written.size(), 1, 10000);
        QVERIFY(written[0][0].toBool()); QVERIFY(!store.busy());
        QVERIFY(store.read()); QTRY_COMPARE_WITH_TIMEOUT(read.size(), 1, 10000);
        const auto result = qvariant_cast<SecretResult>(read[0][0]);
        QVERIFY(result.error.isEmpty());
        // Avoid exposing even synthetic secret contents in failure output.
        QVERIFY(result.key == "synthetic-test-key");
        QVERIFY(store.remove()); QTRY_COMPARE_WITH_TIMEOUT(written.size(), 2, 10000);
        QVERIFY(written[1][0].toBool());
        QVERIFY(store.read()); QTRY_COMPARE_WITH_TIMEOUT(read.size(), 2, 10000);
        QVERIFY(qvariant_cast<SecretResult>(read[1][0]).key.isEmpty());
    }
};
QTEST_GUILESS_MAIN(TestSecret)
#include "TestSecret.moc"
