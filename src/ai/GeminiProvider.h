#pragma once
#include "HttpAIProvider.h"
namespace ws {
class GeminiProvider : public HttpAIProvider {
public:
    explicit GeminiProvider(QObject* parent=nullptr,QUrl endpoint=QUrl("https://generativelanguage.googleapis.com/v1beta/interactions"),int timeoutMs=30000);
protected:
    QJsonObject payload(const AIRequest&)const override;
    void authorize(QNetworkRequest&,const QString&)const override;
    AIResult parse(int,QNetworkReply::NetworkError,const QByteArray&)const override;
};
}
