#include "InstanceCoordinator.h"
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTimer>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif
#ifdef Q_OS_LINUX
#include <sys/socket.h>
#endif
namespace ws {
InstanceCoordinator::InstanceCoordinator(QString directory,QObject* parent):QObject(parent),directory_(std::move(directory)) {
    connect(&server_,&QLocalServer::newConnection,this,[this] {
        while(server_.hasPendingConnections()) {
            auto* socket=server_.nextPendingConnection(); socket->setParent(&server_);
#ifdef Q_OS_LINUX
            ucred credentials{}; socklen_t size=sizeof(credentials);
            if(getsockopt(int(socket->socketDescriptor()),SOL_SOCKET,SO_PEERCRED,&credentials,&size)!=0 || credentials.uid!=geteuid()) {
                socket->abort(); socket->deleteLater(); continue;
            }
#endif
            if(server_.findChildren<QLocalSocket*>().size()>16) {socket->abort();socket->deleteLater();continue;}
            socket->setReadBufferSize(32);
            auto* deadline=new QTimer(socket); deadline->setSingleShot(true); deadline->start(2000);
            connect(deadline,&QTimer::timeout,socket,[socket]{socket->abort();socket->deleteLater();});
            connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
            connect(socket,&QLocalSocket::readyRead,this,[this,socket,deadline] {
                if(socket->property("handled").toBool()) return;
                QByteArray buffer=socket->property("command").toByteArray()+socket->readAll();
                if(buffer.size()>16) {socket->abort();return;}
                socket->setProperty("command",buffer);
                if(!buffer.contains('\n')) return;
                socket->setProperty("handled",true); deadline->stop();
                if(buffer!="trigger\n" && buffer!="open\n") {socket->abort();return;}
                socket->write("ok\n"); socket->flush(); socket->disconnectFromServer();
                if(buffer=="trigger\n") emit triggerRequested(); else emit openRequested();
            });
        }
    });
}
bool InstanceCoordinator::prepareDirectory() {
    if(directory_.isEmpty()) {
        const auto runtime=QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if(runtime.isEmpty()) {error_="No user runtime directory; check XDG_RUNTIME_DIR.";return false;}
        directory_=QDir(runtime).filePath("worksidekick");
    }
    QFileInfo before(directory_);
    if(before.isSymLink()) {error_="Unsafe IPC directory symlink.";return false;}
    if(!QDir().mkpath(directory_)) {error_="Cannot create user IPC directory.";return false;}
    QFileInfo info(directory_);
#ifdef Q_OS_UNIX
    if(info.ownerId()!=uint(geteuid())) {error_="IPC directory belongs to another user.";return false;}
#endif
    const auto privatePermissions=QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner;
    if(!QFile::setPermissions(directory_,privatePermissions)) {error_="Cannot secure IPC directory.";return false;}
    endpoint_=QDir(directory_).filePath("command.sock");
    return true;
}
bool InstanceCoordinator::forward(bool trigger,bool& connected) {
    QLocalSocket socket; socket.connectToServer(endpoint_);
    connected=socket.waitForConnected(300);
    if(!connected) {
        if(socket.error()!=QLocalSocket::ServerNotFoundError && socket.error()!=QLocalSocket::ConnectionRefusedError)
            error_="Cannot access existing instance socket; no endpoint removed.";
        return false;
    }
    socket.write(trigger?"trigger\n":"open\n");
    if(socket.bytesToWrite()>0 && !socket.waitForBytesWritten(1500)) {error_="Existing instance did not accept command.";return false;}
    QByteArray ack;
    while(!ack.contains('\n') && ack.size()<16) {
        if(!socket.bytesAvailable() && !socket.waitForReadyRead(1500)) break;
        ack+=socket.readAll();
    }
    if(ack!="ok\n") {error_="Existing instance did not acknowledge; retry when it is responsive.";return false;}
    return true;
}
InstanceCoordinator::Result InstanceCoordinator::start(bool trigger) {
    if(server_.isListening()) return Result::Primary;
    error_.clear();
    if(!prepareDirectory()) return Result::Failed;
    bool connected=false;
    if(forward(trigger,connected)) return Result::Forwarded;
    if(connected || !error_.isEmpty()) return Result::Failed;
    lock_=std::make_unique<QLockFile>(QDir(directory_).filePath("instance.lock"));
    lock_->setStaleLockTime(0); // Only dead-owner recovery, never evict a slow live process.
    if(!lock_->tryLock(0)) {
        if(forward(trigger,connected)) return Result::Forwarded;
        error_="Another instance is starting or unresponsive. Retry --trigger; no socket removed.";
        lock_.reset(); return Result::Failed;
    }
    // Serialize stale-endpoint cleanup with the primary lifetime lock.
    if(forward(trigger,connected)) {lock_.reset();return Result::Forwarded;}
    if(connected || !error_.isEmpty()) {lock_.reset();return Result::Failed;}
    if(QFileInfo::exists(endpoint_) && !QLocalServer::removeServer(endpoint_)) {
        error_="Cannot remove stale user socket.";lock_.reset();return Result::Failed;
    }
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    if(!server_.listen(endpoint_)) {error_="Cannot listen on user IPC socket.";lock_.reset();return Result::Failed;}
    return Result::Primary;
}
}
