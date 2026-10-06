#include <QtTest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QFileInfo>
#include "app/InstanceCoordinator.h"
using namespace ws;
class TestInstance : public QObject {
    Q_OBJECT
private slots:
    void productionCliForwardingAndColdStart() {
        QTemporaryDir runtime;
        auto environment=QProcessEnvironment::systemEnvironment();
        environment.insert("XDG_RUNTIME_DIR",runtime.path());
        environment.insert("XDG_CONFIG_HOME",runtime.filePath("config"));
        const auto directory=QCoreApplication::applicationDirPath();
        QProcess primary; primary.setProcessEnvironment(environment);
        primary.start(directory+"/instance_peer",{"--default"});
        QVERIFY(primary.waitForStarted()); QByteArray output;
        QTRY_VERIFY_WITH_TIMEOUT(([&]{output+=primary.readAllStandardOutput();return output.contains("primary");})(),5000);
        QProcess client; client.setProcessEnvironment(environment);
        client.start(directory+"/worksidekick",{"--trigger"}); QVERIFY(client.waitForFinished(5000)); QCOMPARE(client.exitCode(),0);
        QTRY_VERIFY(([&]{output+=primary.readAllStandardOutput();return output.contains("trigger");})());
        QCOMPARE(output.count("trigger"),1);
        client.start(directory+"/worksidekick",{}); QVERIFY(client.waitForFinished(5000)); QCOMPARE(client.exitCode(),0);
        QTRY_VERIFY(([&]{output+=primary.readAllStandardOutput();return output.contains("open");})());
        primary.kill(); QVERIFY(primary.waitForFinished());
        QProcess cold; cold.setProcessEnvironment(environment); cold.start(directory+"/worksidekick",{"--trigger"});
        QVERIFY(cold.waitForStarted());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(runtime.filePath("worksidekick/command.sock")),5000);
        // Allow event-loop startup; a real second command must be acknowledged by the app.
        QTest::qWait(200);
        client.start(directory+"/worksidekick",{"--trigger"}); QVERIFY(client.waitForFinished(5000)); QCOMPARE(client.exitCode(),0);
        QCOMPARE(cold.state(),QProcess::Running);
        cold.terminate(); if(!cold.waitForFinished(3000)) {cold.kill();cold.waitForFinished();}
    }

    void simultaneousStartupKeepsOnePrimary() {
        QTemporaryDir dir;
        const auto exe=QCoreApplication::applicationDirPath()+"/instance_peer";
        QProcess a,b; a.start(exe,{dir.filePath("ipc")}); b.start(exe,{dir.filePath("ipc")});
        QVERIFY(a.waitForStarted()); QVERIFY(b.waitForStarted());
        QByteArray first,second;
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            first+=a.readAllStandardOutput(); second+=b.readAllStandardOutput();
            return (first.contains("primary") || second.contains("primary")) &&
                (a.state()==QProcess::NotRunning || b.state()==QProcess::NotRunning);
        })(),5000);
        QCOMPARE(first.count("primary")+second.count("primary"),1);
        auto* owner=first.contains("primary")?&a:&b;
        auto* other=owner==&a?&b:&a;
        QCOMPARE(owner->state(),QProcess::Running);
        QVERIFY(other->exitCode()==0 || other->exitCode()==2);
        // A losing startup may ask for retry, but must leave the primary reachable.
        QProcess retry; retry.start(exe,{dir.filePath("ipc"),"--trigger"});
        QVERIFY(retry.waitForFinished(5000)); QCOMPARE(retry.exitCode(),0);
        owner->kill(); QVERIFY(owner->waitForFinished());
    }
    void permissionsAndProtocol() {
        QTemporaryDir dir; InstanceCoordinator instance(dir.filePath("ipc"));
        QCOMPARE(instance.start(false),InstanceCoordinator::Result::Primary);
        auto permissions=QFileInfo(dir.filePath("ipc")).permissions();
        QVERIFY(!(permissions & (QFileDevice::ReadGroup|QFileDevice::WriteGroup|QFileDevice::ExeGroup|
                                 QFileDevice::ReadOther|QFileDevice::WriteOther|QFileDevice::ExeOther)));
        QSignalSpy triggered(&instance,&InstanceCoordinator::triggerRequested);
        QLocalSocket bad; bad.connectToServer(instance.endpoint()); QVERIFY(bad.waitForConnected());
        bad.write("trigger secret-text-invalid\n"); bad.flush();
        QTRY_COMPARE(bad.state(),QLocalSocket::UnconnectedState); QCOMPARE(triggered.size(),0);
        QLocalSocket valid; valid.connectToServer(instance.endpoint()); QVERIFY(valid.waitForConnected());
        valid.write("trigger\n"); valid.flush();
        QTRY_COMPARE(triggered.size(),1);
        QTRY_VERIFY(valid.bytesAvailable()>0); QCOMPARE(valid.readAll(),QByteArray("ok\n"));
    }
    void subprocessForwardingAndStaleLock() {
        QTemporaryDir dir;
        const auto exe=QCoreApplication::applicationDirPath()+"/instance_peer";
        QProcess primary; primary.start(exe,{dir.filePath("ipc")}); QVERIFY(primary.waitForStarted());
        QByteArray output;
        QTRY_VERIFY_WITH_TIMEOUT(([&]{output+=primary.readAllStandardOutput();return output.contains("primary");})(),5000);
        QProcess client; client.start(exe,{dir.filePath("ipc"),"--trigger"}); QVERIFY(client.waitForFinished(5000)); QCOMPARE(client.exitCode(),0);
        QTRY_VERIFY(([&]{output+=primary.readAllStandardOutput();return output.contains("trigger");})());
        QCOMPARE(output.count("trigger"),1);
        client.start(exe,{dir.filePath("ipc")}); QVERIFY(client.waitForFinished(5000)); QCOMPARE(client.exitCode(),0);
        QTRY_VERIFY(([&]{output+=primary.readAllStandardOutput();return output.contains("open");})());
        primary.kill(); QVERIFY(primary.waitForFinished());
        // Dead process leaves a lock/socket; recovery must be safe and automatic.
        InstanceCoordinator recovered(dir.filePath("ipc"));
        QCOMPARE(recovered.start(false),InstanceCoordinator::Result::Primary);
    }
};
QTEST_GUILESS_MAIN(TestInstance)
#include "TestInstance.moc"
