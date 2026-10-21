#include "WindowsSecretStore.h"
#include <QtConcurrent/QtConcurrentRun>
#include <windows.h>
#include <wincred.h>
namespace ws {
namespace {
SecretResult operate(int operation,const QString& target,const QString& key) {
    const auto name=target.toStdWString();
    const QString failure="Windows Credential Manager is unavailable. Check your Windows sign-in and retry.";
    if(operation==0) {
        PCREDENTIALW value=nullptr;
        if(!CredReadW(name.c_str(),CRED_TYPE_GENERIC,0,&value))return {{},GetLastError()==ERROR_NOT_FOUND?QString{}:failure};
        QString result;
        const bool valid=value->CredentialBlobSize<=CRED_MAX_CREDENTIAL_BLOB_SIZE && value->CredentialBlobSize%sizeof(wchar_t)==0;
        if(valid)result=QString::fromWCharArray(reinterpret_cast<const wchar_t*>(value->CredentialBlob),int(value->CredentialBlobSize/sizeof(wchar_t)));
        if(value->CredentialBlob)SecureZeroMemory(value->CredentialBlob,value->CredentialBlobSize);
        CredFree(value);return {result,valid?QString{}:failure};
    }
    if(operation==2) {
        const bool ok=CredDeleteW(name.c_str(),CRED_TYPE_GENERIC,0) || GetLastError()==ERROR_NOT_FOUND;
        return {{},ok?QString{}:failure};
    }
    auto bytes=key.toStdWString();
    if(bytes.size()*sizeof(wchar_t)>CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        SecureZeroMemory(bytes.data(),bytes.size()*sizeof(wchar_t));return {{},"API key exceeds Windows Credential Manager's size limit."};
    }
    CREDENTIALW value{};value.Type=CRED_TYPE_GENERIC;value.TargetName=const_cast<wchar_t*>(name.c_str());
    value.CredentialBlobSize=DWORD(bytes.size()*sizeof(wchar_t));value.CredentialBlob=reinterpret_cast<LPBYTE>(bytes.data());
    value.Persist=CRED_PERSIST_LOCAL_MACHINE;
    const bool ok=CredWriteW(&value,0);SecureZeroMemory(bytes.data(),bytes.size()*sizeof(wchar_t));
    return {{},ok?QString{}:failure};
}
}
WindowsSecretStore::WindowsSecretStore(QObject* parent,QString account,QString prefix)
    :ISecretStore(parent),target_(prefix+account){}
bool WindowsSecretStore::busy()const{return active_;}
QString WindowsSecretStore::description()const{return "Windows Credential Manager (current user, local computer)";}
bool WindowsSecretStore::read(){return start(0);}
bool WindowsSecretStore::save(const QString& key){return !key.trimmed().isEmpty() && start(1,key.trimmed());}
bool WindowsSecretStore::remove(){return start(2);}
bool WindowsSecretStore::start(int operation,const QString& key) {
    if(active_)return false;
    active_=true;disconnect(&watcher_,nullptr,this,nullptr);
    connect(&watcher_,&QFutureWatcher<SecretResult>::finished,this,[this,operation] {
        const auto result=watcher_.result();active_=false;
        if(operation==0)emit readFinished(result);
        else emit writeFinished(result.error.isEmpty(),result.error.isEmpty()?QString("Secure key storage updated."):result.error);
    });
    watcher_.setFuture(QtConcurrent::run([operation,target=target_,key]{return operate(operation,target,key);}));return true;
}
}
