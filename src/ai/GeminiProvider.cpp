#include "GeminiProvider.h"
namespace ws {
GeminiProvider::GeminiProvider(QObject* p,QUrl e,int t):HttpAIProvider("Gemini",e,t,p){}
QJsonObject GeminiProvider::payload(const AIRequest&)const{return {};}
void GeminiProvider::authorize(QNetworkRequest&,const QString&)const{}
AIResult GeminiProvider::parse(int,QNetworkReply::NetworkError,const QByteArray&)const{return {};}
}
