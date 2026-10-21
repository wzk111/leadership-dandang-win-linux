#include "InstanceCoordinator.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTimer>
#include <QElapsedTimer>
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
namespace ws {
namespace {
QByteArray tokenUser(HANDLE process) {
    HANDLE token=nullptr;if(!OpenProcessToken(process,TOKEN_QUERY,&token))return {};
    DWORD size=0;GetTokenInformation(token,TokenUser,nullptr,0,&size);QByteArray data(int(size),0);
    if(!GetTokenInformation(token,TokenUser,data.data(),size,&size))data.clear();CloseHandle(token);return data;
}
QString sidText() {
    const auto data=tokenUser(GetCurrentProcess());if(data.isEmpty())return {};
    LPWSTR text=nullptr;if(!ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(data.constData())->User.Sid,&text))return {};
    const auto result=QString::fromWCharArray(text);LocalFree(text);return result;
}
bool sameUserServer(QLocalSocket& socket) {
    ULONG pid=0;
    if(!GetNamedPipeServerProcessId(reinterpret_cast<HANDLE>(socket.socketDescriptor()),&pid))return false;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;
    const auto peer=tokenUser(process);CloseHandle(process);const auto self=tokenUser(GetCurrentProcess());
    DWORD peerSession=0,selfSession=0;
    return !peer.isEmpty() && !self.isEmpty() &&
        ProcessIdToSessionId(pid,&peerSession) && ProcessIdToSessionId(GetCurrentProcessId(),&selfSession) && peerSession==selfSession &&
        EqualSid(reinterpret_cast<const TOKEN_USER*>(peer.constData())->User.Sid,reinterpret_cast<const TOKEN_USER*>(self.constData())->User.Sid);
}
}
InstanceCoordinator::InstanceCoordinator(QString directory,QObject* parent):QObject(parent),directory_(std::move(directory)) {
    server_.setMaxPendingConnections(16);
    connect(&server_,&QLocalServer::newConnection,this,[this] {
        while(server_.hasPendingConnections()) {
            auto* socket=server_.nextPendingConnection();socket->setParent(&server_);
            if(server_.findChildren<QLocalSocket*>().size()>16){socket->abort();socket->deleteLater();continue;}
            socket->setReadBufferSize(17);
            auto* deadline=new QTimer(socket);deadline->setSingleShot(true);deadline->start(2000);
            connect(deadline,&QTimer::timeout,socket,[socket]{socket->abort();socket->deleteLater();});
            connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
            connect(socket,&QLocalSocket::readyRead,this,[this,socket,deadline] {
                if(socket->property("handled").toBool())return;
                const QByteArray buffer=socket->property("command").toByteArray()+socket->readAll();
                if(buffer.size()>16){socket->abort();return;}
                socket->setProperty("command",buffer);if(!buffer.contains('\n'))return;
                socket->setProperty("handled",true);
                if(buffer!="trigger\n" && buffer!="open\n"){socket->abort();return;}
                socket->write("ok\n");socket->flush();socket->disconnectFromServer();
                // Retain the deadline until disconnection, including a stalled ack reader.
                if(buffer=="trigger\n")emit triggerRequested();else emit openRequested();
            });
        }
    });
}
bool InstanceCoordinator::prepareDirectory() {
    const auto sid=sidText();DWORD session=0;
    if(sid.isEmpty() || !ProcessIdToSessionId(GetCurrentProcessId(),&session)){error_="Cannot determine current Windows user/session.";return false;}
    if(directory_.isEmpty())directory_=QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("WorkSidekick/runtime");
    directory_=QDir::cleanPath(QFileInfo(directory_).absoluteFilePath());
    if(!QDir().mkpath(QFileInfo(directory_).absolutePath())){error_="Cannot create local IPC parent directory.";return false;}
    // Protected ACL grants access only to this user's SID, with inheritance for lock files.
    const auto sddl=("D:P(A;OICI;FA;;;"+sid+")").toStdWString();PSECURITY_DESCRIPTOR descriptor=nullptr;
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&descriptor,nullptr)){error_="Cannot create private IPC security descriptor.";return false;}
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),descriptor,FALSE};
    const auto path=QDir::toNativeSeparators(directory_).toStdWString();
    const bool created=CreateDirectoryW(path.c_str(),&attributes);
    if(!created && GetLastError()!=ERROR_ALREADY_EXISTS){LocalFree(descriptor);error_="Cannot create private IPC directory.";return false;}
    HANDLE folder=CreateFileW(path.c_str(),READ_CONTROL|WRITE_DAC,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    bool secure=folder!=INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION info{};
    if(secure)secure=GetFileInformationByHandle(folder,&info) && (info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && !(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT);
    PSID owner=nullptr;PSECURITY_DESCRIPTOR existing=nullptr;const auto self=tokenUser(GetCurrentProcess());
    if(secure)secure=GetSecurityInfo(folder,SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&owner,nullptr,nullptr,nullptr,&existing)==ERROR_SUCCESS &&
        !self.isEmpty() && EqualSid(owner,reinterpret_cast<const TOKEN_USER*>(self.constData())->User.Sid);
    PACL acl=nullptr;BOOL present=FALSE,defaulted=FALSE;
    if(secure)secure=GetSecurityDescriptorDacl(descriptor,&present,&acl,&defaulted) && present && acl &&
        SetSecurityInfo(folder,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,acl,nullptr)==ERROR_SUCCESS;
    if(existing)LocalFree(existing);if(folder!=INVALID_HANDLE_VALUE)CloseHandle(folder);LocalFree(descriptor);
    if(!secure){error_="IPC directory is not a private, current-user directory.";return false;}
    directory_=QFileInfo(directory_).canonicalFilePath();
    const auto identity=sid+"/"+QString::number(session)+"/"+directory_.toCaseFolded();
    endpoint_="WorkSidekick-"+QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(),QCryptographicHash::Sha256).toHex());
    return true;
}
bool InstanceCoordinator::forward(bool trigger,bool& connected) {
    QLocalSocket socket;socket.connectToServer(endpoint_);connected=socket.waitForConnected(300);
    if(!connected) {
        if(socket.error()!=QLocalSocket::ServerNotFoundError && socket.error()!=QLocalSocket::ConnectionRefusedError)
            error_="Cannot access existing instance pipe.";
        return false;
    }
    if(!sameUserServer(socket)){error_="Existing instance is not owned by this Windows user/session.";return false;}
    socket.write(trigger?"trigger\n":"open\n");
    if(socket.bytesToWrite()>0 && !socket.waitForBytesWritten(1500)){error_="Existing instance did not accept command.";return false;}
    QByteArray ack;QElapsedTimer timeout;timeout.start();
    while(!ack.contains('\n') && ack.size()<=16 && timeout.elapsed()<1500) {
        if(!socket.bytesAvailable() && !socket.waitForReadyRead(int(1500-timeout.elapsed())))break;
        ack+=socket.read(17-ack.size());
    }
    if(ack!="ok\n"){error_="Existing instance did not acknowledge; retry when responsive.";return false;}
    return true;
}
InstanceCoordinator::Result InstanceCoordinator::start(bool trigger) {
    if(server_.isListening())return Result::Primary;
    error_.clear();if(!prepareDirectory())return Result::Failed;
    bool connected=false;if(forward(trigger,connected))return Result::Forwarded;
    if(connected || !error_.isEmpty())return Result::Failed;
    lock_=std::make_unique<QLockFile>(QDir(directory_).filePath(endpoint_+".lock"));lock_->setStaleLockTime(0);
    if(!lock_->tryLock(0)) {
        if(forward(trigger,connected))return Result::Forwarded;
        error_="Another instance is starting or unresponsive. Retry; its lock is preserved.";lock_.reset();return Result::Failed;
    }
    if(forward(trigger,connected)){lock_.reset();return Result::Forwarded;}
    if(connected || !error_.isEmpty()){lock_.reset();return Result::Failed;}
    // Named pipe instances disappear with their owner; there is no filesystem socket to unlink.
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    if(!server_.listen(endpoint_)){error_="Cannot listen on private instance pipe.";lock_.reset();return Result::Failed;}
    return Result::Primary;
}
}
