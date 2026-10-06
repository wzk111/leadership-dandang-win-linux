#include "HttpAIProvider.h"
namespace ws {
HttpAIProvider::HttpAIProvider(QString id,QUrl endpoint,int timeoutMs,QObject* parent):IAIProvider(parent),provider_(id),endpoint_(endpoint),timeoutMs_(timeoutMs){}
bool HttpAIProvider::generate(const AIRequest&){return false;}
void HttpAIProvider::cancel(){}
bool HttpAIProvider::busy()const{return false;}
AIResult HttpAIProvider::failure(AIError e,const QString&){return {{},e,"Unavailable"};}
QUrl HttpAIProvider::endpointFor(const AIRequest&)const{return endpoint_;}
void HttpAIProvider::finishLater(AIError){}
}
