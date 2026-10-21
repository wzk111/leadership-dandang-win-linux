#include "OpenAICompatibleProvider.h"
#include "OpenAIProvider.h"
#include <QNetworkRequest>
#include <algorithm>
namespace ws {
OpenAICompatibleProvider::OpenAICompatibleProvider(QObject* p,int t,bool loop)
    :HttpAIProvider("OpenAI-compatible",{},t,p),allowLoopback_(loop){}
QJsonObject OpenAICompatibleProvider::payload(const AIRequest& r)const {
    return {{"model",r.model.trimmed()},{"instructions",r.prompt.system},{"input",r.prompt.user},
        {"store",false},{"max_output_tokens",std::clamp(r.maxOutputTokens,1,16384)}};
}
void OpenAICompatibleProvider::authorize(QNetworkRequest& request,const QString& key)const {
    request.setRawHeader("Authorization","Bearer "+key.toUtf8());
}
AIResult OpenAICompatibleProvider::parse(int status,QNetworkReply::NetworkError error,const QByteArray& body)const {
    return OpenAIProvider::decode(status,error,body);
}
QUrl OpenAICompatibleProvider::endpointFor(const AIRequest& r)const {
    QUrl url(r.baseUrl.trimmed(),QUrl::StrictMode);
    if(!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
       url.hasQuery() || url.hasFragment() ||
       (url.scheme()!="https" && !(allowLoopback_ && url.scheme()=="http" && url.host()=="127.0.0.1")))return {};
    auto path=url.path();
    while(path.endsWith('/'))path.chop(1);
    url.setPath(path+"/responses");
    return url;
}
}
