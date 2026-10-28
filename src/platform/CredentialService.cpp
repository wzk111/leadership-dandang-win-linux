#include "CredentialService.h"
#include <QStringList>
namespace ws {
CredentialService::CredentialService(QMap<QString,ISecretStore*> stores,QObject* p):ISecretStore(p),stores_(stores) {
    for(auto it=stores_.cbegin();it!=stores_.cend();++it) {
        auto* store=it.value(); const auto provider=it.key();
        if(!store)continue;
        store->setParent(this);
        connect(store,&ISecretStore::readFinished,this,[this,provider](const SecretResult& result) {
            if(!active_ || selected_!=provider || operation_!=0)return;
            active_=false;operation_=-1;
            status_[provider]=!result.error.isEmpty()?"unavailable":result.key.isEmpty()?"not saved":"saved";
            // Backend errors must not expose credentials or provider attributes.
            emit readFinished(result.error.isEmpty()?result:SecretResult{{},"Secret store unavailable or locked. Check your OS credential storage and retry."});
        });
        connect(store,&ISecretStore::writeFinished,this,[this,provider](bool success,const QString&) {
            if(!active_ || selected_!=provider || (operation_!=1 && operation_!=2))return;
            const auto operation=operation_;active_=false;operation_=-1;
            status_[provider]=success?(operation==1?"saved":"not saved"):"unavailable";
            emit writeFinished(success,success?"Secure key storage updated.":"Secret store unavailable or locked. Check your OS credential storage and retry.");
        });
    }
}
bool CredentialService::selectProvider(const QString& provider) {
    if(busy() || !QStringList{"openai","anthropic","gemini","openai_compatible"}.contains(provider) || !stores_.value(provider))return false;
    selected_=provider;return true;
}
bool CredentialService::read(){return start(0);}
bool CredentialService::save(const QString& key){return !key.trimmed().isEmpty() && start(1,key.trimmed());}
bool CredentialService::remove(){return start(2);}
bool CredentialService::busy()const{return active_;}
QString CredentialService::description()const{
    const auto* store=stores_.value(selected_);
    return store?"Provider-specific credentials: "+store->description():"Credential backend unavailable";
}
QString CredentialService::credentialStatus(const QString& provider)const{return status_.value(provider,"not checked");}
bool CredentialService::start(int operation,const QString& key) {
    auto* store=stores_.value(selected_);
    if(active_ || !store || store->busy())return false;
    active_=true;operation_=operation;
    const bool started=operation==0?store->read():operation==1?store->save(key):store->remove();
    if(!started){active_=false;operation_=-1;}
    return started;
}
}
