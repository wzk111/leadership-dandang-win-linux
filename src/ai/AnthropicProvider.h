#pragma once
#include "HttpAIProvider.h"
namespace ws {
class AnthropicProvider : public HttpAIProvider {
public:
    explicit AnthropicProvider(QObject* parent=nullptr,QUrl endpoint=QUrl("https://api.anthropic.com/v1/messages"),int timeoutMs=30000);
protected:
    QJsonObject payload(const AIRequest&)const override;
    void authorize(QNetworkRequest&,const QString&)const override;
    AIResult parse(int,QNetworkReply::NetworkError,const QByteArray&)const override;
};
}
