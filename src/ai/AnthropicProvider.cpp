#include "AnthropicProvider.h"
namespace ws {
AnthropicProvider::AnthropicProvider(QObject* p,QUrl e,int t):HttpAIProvider("Anthropic",e,t,p){}
QJsonObject AnthropicProvider::payload(const AIRequest&)const{return {};}
void AnthropicProvider::authorize(QNetworkRequest&,const QString&)const{}
AIResult AnthropicProvider::parse(int,QNetworkReply::NetworkError,const QByteArray&)const{return {};}
}
