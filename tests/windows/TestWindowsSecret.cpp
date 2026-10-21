#include <QtTest>
#include <QUuid>
#include <array>
#include <memory>
#include <windows.h>
#include <wincred.h>
#include "platform/windows/WindowsSecretStore.h"
using namespace ws;
class TestWindowsSecret:public QObject {
 Q_OBJECT
private slots:
 void isolatedRoundTrip() {
    const auto prefix="WorkSidekick/test-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+"/";
    const QStringList ids{"openai","anthropic","gemini","openai_compatible"};
    struct Cleanup {
        QString prefix;QStringList ids;
        ~Cleanup(){for(const auto& id:ids){auto target=(prefix+id).toStdWString();CredDeleteW(target.c_str(),CRED_TYPE_GENERIC,0);}}
    } cleanup{prefix,ids};
    std::array<std::unique_ptr<WindowsSecretStore>,4> stores;
    for(int i=0;i<int(stores.size());++i) {
        stores[i]=std::make_unique<WindowsSecretStore>(nullptr,ids[i],prefix);
        auto& store=*stores[i];QSignalSpy writes(&store,&ISecretStore::writeFinished);
        QVERIFY(store.save("synthetic credential 中文 "+ids[i]));QVERIFY(store.busy());QVERIFY(!store.read());
        QTRY_COMPARE(writes.size(),1);QVERIFY(writes[0][0].toBool());
        PCREDENTIALW raw=nullptr;const auto target=(prefix+ids[i]).toStdWString();
        QVERIFY(CredReadW(target.c_str(),CRED_TYPE_GENERIC,0,&raw));
        const auto persistence=raw->Persist;CredFree(raw);QCOMPARE(persistence,DWORD(CRED_PERSIST_LOCAL_MACHINE));
    }
    for(int i=0;i<int(stores.size());++i) {
        QSignalSpy reads(stores[i].get(),&ISecretStore::readFinished);
        QVERIFY(stores[i]->read());QTRY_COMPARE(reads.size(),1);
        const auto result=qvariant_cast<SecretResult>(reads[0][0]);QVERIFY(result.error.isEmpty());
        QCOMPARE(result.key,"synthetic credential 中文 "+ids[i]);
    }
    // Deleting Anthropic must leave all three other native Credential Manager targets intact.
    QSignalSpy removed(stores[1].get(),&ISecretStore::writeFinished);
    QVERIFY(stores[1]->remove());QTRY_COMPARE(removed.size(),1);QVERIFY(removed[0][0].toBool());
    for(int i=0;i<int(stores.size());++i) {
        QSignalSpy reads(stores[i].get(),&ISecretStore::readFinished);
        QVERIFY(stores[i]->read());QTRY_COMPARE(reads.size(),1);
        const auto result=qvariant_cast<SecretResult>(reads[0][0]);QVERIFY(result.error.isEmpty());
        QCOMPARE(result.key,i==1?QString{}:"synthetic credential 中文 "+ids[i]);
        QVERIFY(!stores[i]->save(" "));QVERIFY(!stores[i]->description().contains("synthetic"));
    }
 }
};
QTEST_GUILESS_MAIN(TestWindowsSecret)
#include "TestWindowsSecret.moc"
