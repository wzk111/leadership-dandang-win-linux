#include "OpenAICompatibleProvider.h"
namespace ws {
OpenAICompatibleProvider::OpenAICompatibleProvider(QObject* p,int t,bool loop):HttpAIProvider("OpenAI-compatible",{},t,p),allowLoopback_(loop){}
QJsonObject OpenAICompatibleProvider::payload(const AIRequest&)const{return {};}
void OpenAICompatibleProvider::authorize(QNetworkRequest&,const QString&)const{}
AIResult OpenAICompatibleProvider::parse(int,QNetworkReply::NetworkError,const QByteArray&)const{return {};}
QUrl OpenAICompatibleProvider::endpointFor(const AIRequest&)const{return {};}
}
