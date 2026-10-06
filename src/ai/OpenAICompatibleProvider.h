#pragma once
#include "HttpAIProvider.h"
namespace ws {
class OpenAICompatibleProvider : public HttpAIProvider {
public:
    explicit OpenAICompatibleProvider(QObject* parent=nullptr,int timeoutMs=30000,bool allowLoopbackForTests=false);
protected:
    QJsonObject payload(const AIRequest&)const override;
    void authorize(QNetworkRequest&,const QString&)const override;
    AIResult parse(int,QNetworkReply::NetworkError,const QByteArray&)const override;
    QUrl endpointFor(const AIRequest&)const override;
private:
    bool allowLoopback_;
};
}
