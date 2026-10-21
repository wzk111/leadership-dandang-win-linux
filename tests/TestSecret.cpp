#include <QtTest>
#include <QUuid>
#include <QScopeGuard>
#include "platform/CredentialService.h"
#include "platform/linux/LinuxSecretStore.h"
using namespace ws;
class TestSecret : public QObject {
    Q_OBJECT
private slots:
    void providerIsolation() {
        const auto prefix="test-"+QUuid::createUuid().toString()+"-";
        QMap<QString,ISecretStore*> stores;
        const QStringList providers{"openai","anthropic","gemini","openai_compatible"};
        for(const auto& id:providers)stores.insert(id,new LinuxSecretStore(nullptr,prefix+id));
        CredentialService service(stores);
        QSignalSpy written(&service,&ISecretStore::writeFinished),read(&service,&ISecretStore::readFinished);
        const auto cleanup=qScopeGuard([&] {
            for(auto* store:stores) {
                QSignalSpy done(store,&ISecretStore::writeFinished);
                if(store->remove())done.wait(10000);
            }
        });
        for(const auto& id:providers) {
            QVERIFY(service.selectProvider(id));QVERIFY(service.save("synthetic-"+id));
            QVERIFY(!service.selectProvider(id));
            QVERIFY(written.wait(10000));QVERIFY(written.last()[0].toBool());
        }
        for(const auto& id:providers) {
            QVERIFY(service.selectProvider(id));QVERIFY(service.read());QVERIFY(read.wait(10000));
            const auto result=qvariant_cast<SecretResult>(read.last()[0]);
            QVERIFY(result.error.isEmpty());QVERIFY(result.key=="synthetic-"+id);
        }
        QVERIFY(service.selectProvider("anthropic"));QVERIFY(service.remove());
        QVERIFY(written.wait(10000));QVERIFY(written.last()[0].toBool());
        for(const auto& id:providers) {
            QVERIFY(service.selectProvider(id));QVERIFY(service.read());QVERIFY(read.wait(10000));
            const auto result=qvariant_cast<SecretResult>(read.last()[0]);
            QVERIFY(result.error.isEmpty());
            QVERIFY(id=="anthropic"?result.key.isEmpty():result.key=="synthetic-"+id);
        }
    }
    void preservesLegacyOpenAIAccount() {
        if(qEnvironmentVariable("WORKSIDEKICK_TEST_ISOLATED_KEYRING")!="1")
            QSKIP("Requires the explicitly isolated CI keyring.");
        LinuxSecretStore legacy;
        QSignalSpy legacyRead(&legacy,&ISecretStore::readFinished),legacyWrite(&legacy,&ISecretStore::writeFinished);
        QVERIFY(legacy.read());QVERIFY(legacyRead.wait(10000));
        const auto existing=qvariant_cast<SecretResult>(legacyRead.last()[0]);
        QVERIFY(existing.error.isEmpty());
        // Never overwrite an existing credential, even with a misconfigured test environment.
        QVERIFY(existing.key.isEmpty());
        const auto cleanup=qScopeGuard([&] {if(legacy.remove())legacyWrite.wait(10000);});
        QVERIFY(legacy.save("synthetic-legacy-openai"));QVERIFY(legacyWrite.wait(10000));
        QVERIFY(legacyWrite.last()[0].toBool());
        CredentialService service({{"openai",new LinuxSecretStore(nullptr,"openai")}});
        QSignalSpy read(&service,&ISecretStore::readFinished);
        QVERIFY(service.selectProvider("openai"));QVERIFY(service.read());QVERIFY(read.wait(10000));
        const auto result=qvariant_cast<SecretResult>(read.last()[0]);
        QVERIFY(result.error.isEmpty());QVERIFY(result.key=="synthetic-legacy-openai");
        QCOMPARE(service.credentialStatus("openai"),QString("saved"));
    }
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
