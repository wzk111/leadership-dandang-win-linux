#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QFileInfo>
#include "app/InstanceCoordinator.h"
using namespace ws;
class TestInstance : public QObject {
    Q_OBJECT
private slots:
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
