#include <QtTest>
#include "platform/CredentialService.h"
using namespace ws;
class Account:public ISecretStore {
public:
    QString key;int reads=0;bool active=false;
    bool read()override{++reads;active=true;return true;}
    void finish(){active=false;emit readFinished({key,{}});}
    bool save(const QString& value)override{key=value;QTimer::singleShot(0,this,[this]{emit writeFinished(true,"Saved");});return true;}
    bool remove()override{key.clear();QTimer::singleShot(0,this,[this]{emit writeFinished(true,"Removed");});return true;}
    bool busy()const override{return active;}
    QString description()const override{return "test account";}
};
class TestCredentials:public QObject{
 Q_OBJECT
private slots:
 void isolatedAccountsAndOperationSnapshot(){
    auto* openai=new Account;auto* anthropic=new Account;auto* gemini=new Account;auto* compatible=new Account;
    openai->key="legacy-openai";
    CredentialService s({{"openai",openai},{"anthropic",anthropic},{"gemini",gemini},{"openai_compatible",compatible}});
    QSignalSpy read(&s,&ISecretStore::readFinished),written(&s,&ISecretStore::writeFinished);
    QVERIFY(s.selectProvider("openai"));QVERIFY(s.read());QVERIFY(!s.selectProvider("anthropic"));QVERIFY(!s.save("wrong"));
    openai->finish();QCOMPARE(read.size(),1);QVERIFY(qvariant_cast<SecretResult>(read[0][0]).key=="legacy-openai");
    QCOMPARE(anthropic->reads,0);QCOMPARE(s.credentialStatus("openai"),QString("saved"));
    QVERIFY(s.selectProvider("anthropic"));QVERIFY(s.save("anthropic-only"));QTRY_COMPARE(written.size(),1);
    QVERIFY(s.selectProvider("gemini"));QVERIFY(s.save("gemini-only"));QTRY_COMPARE(written.size(),2);
    QVERIFY(s.selectProvider("openai_compatible"));QVERIFY(s.save("compatible-only"));QTRY_COMPARE(written.size(),3);
    QVERIFY(openai->key=="legacy-openai");QVERIFY(anthropic->key=="anthropic-only");QVERIFY(gemini->key=="gemini-only");QVERIFY(compatible->key=="compatible-only");
    QVERIFY(s.selectProvider("anthropic"));QVERIFY(s.remove());QTRY_COMPARE(written.size(),4);
    QVERIFY(anthropic->key.isEmpty());QVERIFY(!gemini->key.isEmpty());QVERIFY(!s.selectProvider("unknown"));
    QCOMPARE(s.credentialStatus("anthropic"),QString("not saved"));QVERIFY(!s.description().contains("legacy-openai"));
 }
};
QTEST_GUILESS_MAIN(TestCredentials)
#include "TestCredentials.moc"
