#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QTextStream>
#include <QDir>
#include <QThread>
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include "app/InstanceCoordinator.h"
using namespace ws;
static bool privateAcl(HANDLE handle,SE_OBJECT_TYPE objectType=SE_FILE_OBJECT) {
    PACL acl=nullptr;PSECURITY_DESCRIPTOR descriptor=nullptr;
    const DWORD securityError=GetSecurityInfo(handle,objectType,DACL_SECURITY_INFORMATION,nullptr,nullptr,&acl,nullptr,&descriptor);
    if(securityError!=ERROR_SUCCESS){qWarning("ACL inspection GetSecurityInfo error=%lu",securityError);return false;}
    bool good=acl && acl->AceCount>0;
    if(!good)qWarning("ACL inspection missing/empty DACL");
    HANDLE token=nullptr;DWORD size=0;QByteArray user;
    if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) {GetTokenInformation(token,TokenUser,nullptr,0,&size);user.resize(size);if(!GetTokenInformation(token,TokenUser,user.data(),size,&size))user.clear();CloseHandle(token);}
    if(user.isEmpty()){qWarning("ACL inspection current-user lookup failed");good=false;}
    for(DWORD i=0;acl && i<acl->AceCount;++i) {
        void* raw=nullptr;if(!GetAce(acl,i,&raw)){qWarning("ACL inspection GetAce error=%lu",GetLastError());good=false;break;}
        auto* header=static_cast<ACE_HEADER*>(raw);
        qInfo("ACL ACE index=%lu type=%u flags=%u",i,unsigned(header->AceType),unsigned(header->AceFlags));
        if(header->AceType==ACCESS_ALLOWED_ACE_TYPE) {
            auto* ace=static_cast<ACCESS_ALLOWED_ACE*>(raw);
            const bool same=!user.isEmpty() && EqualSid(&ace->SidStart,reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid);
            LPWSTR sid=nullptr;
            if(ConvertSidToStringSidW(&ace->SidStart,&sid)) {
                qInfo().noquote()<<QString("ACL allow SID=%1 mask=0x%2 currentUser=%3").arg(QString::fromWCharArray(sid)).arg(ace->Mask,0,16).arg(same);
                LocalFree(sid);
            }
            good=good && same;
        }
    }
    LocalFree(descriptor);return good;
}
class TestWindowsInstance:public QObject {
 Q_OBJECT
private slots:
 void delayedQtWriterWithExternalOwner(){
    QTemporaryDir dir;QProcess owner;
    owner.start(QCoreApplication::applicationFilePath(),{"--peer",dir.filePath("ipc"),"--endpoint"});
    struct StopOwner {QProcess& process;~StopOwner(){if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(3000);}}} stop{owner};
    QVERIFY(owner.waitForStarted(3000));QByteArray output;
    QTRY_VERIFY_WITH_TIMEOUT(([&]{output+=owner.readAllStandardOutput();return output.count('\n')>=2 || owner.state()==QProcess::NotRunning;})(),5000);
    const auto lines=output.split('\n');QVERIFY(lines.size()>=3);
    QVERIFY2(lines[0].trimmed()=="primary",owner.readAllStandardError().constData());
    const auto endpoint=QString::fromUtf8(lines[1].trimmed());QVERIFY(endpoint.startsWith("WorkSidekick-"));
    for(int iteration=0;iteration<10;++iteration){
        QLocalSocket client;client.connectToServer(endpoint);QVERIFY(client.waitForConnected(500));
        QCOMPARE(client.write("trigger\n"),qint64(8));
        qInfo("Delayed writer iteration=%d before sleep pending=%lld state=%d available=%lld",iteration,
            static_cast<long long>(client.bytesToWrite()),int(client.state()),static_cast<long long>(client.bytesAvailable()));
        // The separate owner can consume and acknowledge while this client is descheduled.
        QThread::msleep(50);
        const auto pending=client.bytesToWrite();
        qInfo("Delayed writer iteration=%d after sleep pending=%lld state=%d available=%lld",iteration,
            static_cast<long long>(pending),int(client.state()),static_cast<long long>(client.bytesAvailable()));
        const bool written=client.waitForBytesWritten(500);
        QByteArray ack=client.readAll();
        if(ack.size()<3){client.waitForReadyRead(500);ack+=client.readAll();}
        qInfo("Delayed writer iteration=%d waitWritten=%d pending=%lld state=%d ackBytes=%lld earlyFailureCondition=%d",iteration,
            int(written),static_cast<long long>(client.bytesToWrite()),int(client.state()),static_cast<long long>(ack.size()),int(pending>0 && !written));
        QCOMPARE(ack,QByteArray("ok\n"));
    }
 }
 void nativeAclsAndBoundedProtocol() {
    QTemporaryDir dir;const auto path=dir.filePath("ipc");InstanceCoordinator instance(path);
    QVERIFY2(instance.start(false)==InstanceCoordinator::Result::Primary,qPrintable(instance.error()));
    const auto folder=QDir::toNativeSeparators(path).toStdWString();
    HANDLE h=CreateFileW(folder.c_str(),READ_CONTROL,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    QVERIFY(h!=INVALID_HANDLE_VALUE);const bool secureFolder=privateAcl(h);CloseHandle(h);QVERIFY(secureFolder);
    // QLocalSocket's I/O handle need not request READ_CONTROL. Open a native
    // handle with that explicit right to inspect the actual named-pipe DACL.
    const auto pipe=(QStringLiteral("\\\\.\\pipe\\")+instance.endpoint()).toStdWString();
    HANDLE security=CreateFileW(pipe.c_str(),READ_CONTROL|GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
    QVERIFY2(security!=INVALID_HANDLE_VALUE,qPrintable(QString("Open pipe security handle failed: %1").arg(GetLastError())));
    const bool securePipe=privateAcl(security,SE_KERNEL_OBJECT);CloseHandle(security);QVERIFY(securePipe);
    QCoreApplication::processEvents();
    QLocalSocket client;client.connectToServer(instance.endpoint());QVERIFY(client.waitForConnected());
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
    QProcess client;client.start(exe,{"--peer",path,"--trigger"});QVERIFY(client.waitForFinished(5000));QVERIFY2(client.exitCode()==0,client.readAllStandardError().constData());
    QTRY_VERIFY(([&]{ao+=a.readAllStandardOutput();bo+=b.readAllStandardOutput();return (ao+bo).contains("trigger");})());
    client.start(exe,{"--peer",path});QVERIFY(client.waitForFinished(5000));QVERIFY2(client.exitCode()==0,client.readAllStandardError().constData());
    owner->kill();QVERIFY(owner->waitForFinished());InstanceCoordinator recovery(path);
    QVERIFY2(recovery.start(false)==InstanceCoordinator::Result::Primary,qPrintable(recovery.error()));
 }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    const auto args=app.arguments();
    if(args.size()>2 && args[1]=="--peer") {
        InstanceCoordinator instance(args[2]);const auto result=instance.start(args.contains("--trigger"));
        if(result==InstanceCoordinator::Result::Forwarded)return 0;
        if(result!=InstanceCoordinator::Result::Primary){QTextStream(stderr)<<instance.error()<<Qt::endl;return 2;}
        QTextStream(stdout)<<"primary\n"<<Qt::flush;
        if(args.contains("--endpoint"))QTextStream(stdout)<<instance.endpoint()<<'\n'<<Qt::flush;
        QObject::connect(&instance,&InstanceCoordinator::triggerRequested,&app,[]{QTextStream(stdout)<<"trigger\n"<<Qt::flush;});
        QObject::connect(&instance,&InstanceCoordinator::openRequested,&app,[]{QTextStream(stdout)<<"open\n"<<Qt::flush;});
        return app.exec();
    }
    TestWindowsInstance test;return QTest::qExec(&test,argc,argv);
}
#include "TestWindowsInstance.moc"
