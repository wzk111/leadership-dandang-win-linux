#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QTextStream>
#include <QDir>
#include <windows.h>
#include <aclapi.h>
#include "app/InstanceCoordinator.h"
using namespace ws;
static bool privateAcl(HANDLE handle) {
    PACL acl=nullptr;PSECURITY_DESCRIPTOR descriptor=nullptr;
    if(GetSecurityInfo(handle,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&acl,nullptr,&descriptor)!=ERROR_SUCCESS)return false;
    bool good=acl && acl->AceCount>0;
    HANDLE token=nullptr;DWORD size=0;QByteArray user;
    if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) {GetTokenInformation(token,TokenUser,nullptr,0,&size);user.resize(size);if(!GetTokenInformation(token,TokenUser,user.data(),size,&size))user.clear();CloseHandle(token);}
    if(user.isEmpty())good=false;
    for(DWORD i=0;good && i<acl->AceCount;++i) {
        void* raw=nullptr;if(!GetAce(acl,i,&raw)){good=false;break;}
        auto* header=static_cast<ACE_HEADER*>(raw);
        if(header->AceType==ACCESS_ALLOWED_ACE_TYPE) {
            auto* ace=static_cast<ACCESS_ALLOWED_ACE*>(raw);
            good=EqualSid(&ace->SidStart,reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid);
        }
    }
    LocalFree(descriptor);return good;
}
class TestWindowsInstance:public QObject {
 Q_OBJECT
private slots:
 void nativeAclsAndBoundedProtocol() {
    QTemporaryDir dir;const auto path=dir.filePath("ipc");InstanceCoordinator instance(path);
    QCOMPARE(instance.start(false),InstanceCoordinator::Result::Primary);
    const auto folder=QDir::toNativeSeparators(path).toStdWString();
    HANDLE h=CreateFileW(folder.c_str(),READ_CONTROL,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    QVERIFY(h!=INVALID_HANDLE_VALUE);const bool secureFolder=privateAcl(h);CloseHandle(h);QVERIFY(secureFolder);
    QLocalSocket client;client.connectToServer(instance.endpoint());QVERIFY(client.waitForConnected());
    QVERIFY(privateAcl(reinterpret_cast<HANDLE>(client.socketDescriptor())));
    QSignalSpy triggers(&instance,&InstanceCoordinator::triggerRequested);
    client.write("trigger secret-invalid\n");client.flush();QTRY_COMPARE(client.state(),QLocalSocket::UnconnectedState);QCOMPARE(triggers.size(),0);
    QLocalSocket valid;valid.connectToServer(instance.endpoint());QVERIFY(valid.waitForConnected());valid.write("trigger\n");valid.flush();
    QTRY_COMPARE(triggers.size(),1);QTRY_VERIFY(valid.bytesAvailable()>0);QCOMPARE(valid.readAll(),QByteArray("ok\n"));
    QLocalSocket idle;idle.connectToServer(instance.endpoint());QVERIFY(idle.waitForConnected());QTRY_COMPARE_WITH_TIMEOUT(idle.state(),QLocalSocket::UnconnectedState,4000);
 }
 void competingOwnersAndStaleRecovery() {
    QTemporaryDir dir;const auto path=dir.filePath("ipc");const auto exe=QCoreApplication::applicationFilePath();
    QProcess a,b;a.start(exe,{"--peer",path});b.start(exe,{"--peer",path});QVERIFY(a.waitForStarted());QVERIFY(b.waitForStarted());
    QByteArray ao,bo;
    QTRY_VERIFY_WITH_TIMEOUT(([&]{ao+=a.readAllStandardOutput();bo+=b.readAllStandardOutput();return (ao.contains("primary") || bo.contains("primary")) && (a.state()==QProcess::NotRunning || b.state()==QProcess::NotRunning);})(),6000);
    QCOMPARE(ao.count("primary")+bo.count("primary"),1);auto* owner=ao.contains("primary")?&a:&b;
    QProcess client;client.start(exe,{"--peer",path,"--trigger"});QVERIFY(client.waitForFinished(5000));QCOMPARE(client.exitCode(),0);
    QTRY_VERIFY(([&]{ao+=a.readAllStandardOutput();bo+=b.readAllStandardOutput();return (ao+bo).contains("trigger");})());
    client.start(exe,{"--peer",path});QVERIFY(client.waitForFinished(5000));QCOMPARE(client.exitCode(),0);
    owner->kill();QVERIFY(owner->waitForFinished());InstanceCoordinator recovery(path);QCOMPARE(recovery.start(false),InstanceCoordinator::Result::Primary);
 }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    const auto args=app.arguments();
    if(args.size()>2 && args[1]=="--peer") {
        InstanceCoordinator instance(args[2]);const auto result=instance.start(args.contains("--trigger"));
        if(result==InstanceCoordinator::Result::Forwarded)return 0;
        if(result!=InstanceCoordinator::Result::Primary)return 2;
        QTextStream(stdout)<<"primary\n"<<Qt::flush;
        QObject::connect(&instance,&InstanceCoordinator::triggerRequested,&app,[]{QTextStream(stdout)<<"trigger\n"<<Qt::flush;});
        QObject::connect(&instance,&InstanceCoordinator::openRequested,&app,[]{QTextStream(stdout)<<"open\n"<<Qt::flush;});
        return app.exec();
    }
    TestWindowsInstance test;return QTest::qExec(&test,argc,argv);
}
#include "TestWindowsInstance.moc"
