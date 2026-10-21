#include <QtTest>
#include <QUuid>
#include <windows.h>
#include <wincred.h>
#include "platform/windows/WindowsSecretStore.h"
using namespace ws;
class TestWindowsSecret:public QObject {
 Q_OBJECT
private slots:
 void isolatedRoundTrip() {
    const auto prefix="WorkSidekick/test-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+"/";
    WindowsSecretStore a(nullptr,"openai",prefix),b(nullptr,"gemini",prefix);
    struct Cleanup {QString prefix;~Cleanup(){for(const auto& id:{"openai","gemini"}){auto target=(prefix+id).toStdWString();CredDeleteW(target.c_str(),CRED_TYPE_GENERIC,0);}}} cleanup{prefix};
    QSignalSpy writes(&a,&ISecretStore::writeFinished),reads(&a,&ISecretStore::readFinished),other(&b,&ISecretStore::readFinished);
    QVERIFY(a.save("synthetic credential 中文"));QVERIFY(a.busy());QVERIFY(!a.read());
    QTRY_COMPARE(writes.size(),1);QVERIFY(writes[0][0].toBool());
    PCREDENTIALW raw=nullptr;auto target=(prefix+"openai").toStdWString();QVERIFY(CredReadW(target.c_str(),CRED_TYPE_GENERIC,0,&raw));
    const auto persistence=raw->Persist;CredFree(raw);QCOMPARE(persistence,DWORD(CRED_PERSIST_LOCAL_MACHINE));
    QVERIFY(b.read());QTRY_COMPARE(other.size(),1);QVERIFY(qvariant_cast<SecretResult>(other[0][0]).key.isEmpty());
    QVERIFY(a.read());QTRY_COMPARE(reads.size(),1);QCOMPARE(qvariant_cast<SecretResult>(reads[0][0]).key,QString("synthetic credential 中文"));
    QVERIFY(a.remove());QTRY_COMPARE(writes.size(),2);QVERIFY(writes[1][0].toBool());
    QVERIFY(a.read());QTRY_COMPARE(reads.size(),2);QVERIFY(qvariant_cast<SecretResult>(reads[1][0]).key.isEmpty());
    QVERIFY(!a.save(" "));QVERIFY(!a.description().contains("synthetic"));
 }
};
QTEST_GUILESS_MAIN(TestWindowsSecret)
#include "TestWindowsSecret.moc"
