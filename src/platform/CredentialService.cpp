#include "CredentialService.h"
namespace ws {
CredentialService::CredentialService(QMap<QString,ISecretStore*> stores,QObject* p):ISecretStore(p),stores_(stores){for(auto* s:stores_)s->setParent(this);}
bool CredentialService::selectProvider(const QString&){return false;}
bool CredentialService::read(){return false;}
bool CredentialService::save(const QString&){return false;}
bool CredentialService::remove(){return false;}
bool CredentialService::busy()const{return false;}
QString CredentialService::description()const{return "provider credentials";}
QString CredentialService::credentialStatus(const QString&)const{return "not checked";}
bool CredentialService::start(int,const QString&){return false;}
}
