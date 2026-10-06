#pragma once
#include "IAIProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QJsonObject>
namespace ws {
class HttpAIProvider : public IAIProvider {
 Q_OBJECT
public:
    HttpAIProvider(QString provider,QUrl endpoint,int timeoutMs=30000,QObject* parent=nullptr);
    bool generate(const AIRequest&) override;
    void cancel() override;
    bool busy() const override;
    static AIResult failure(AIError,const QString& provider);
protected:
    virtual QJsonObject payload(const AIRequest&) const=0;
    virtual void authorize(QNetworkRequest&,const QString&) const=0;
    virtual AIResult parse(int,QNetworkReply::NetworkError,const QByteArray&) const=0;
    virtual QUrl endpointFor(const AIRequest&) const;
    QString provider_;
private:
    void finishLater(AIError);
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QTimer timer_;
    QUrl endpoint_;
    QByteArray body_;
    int timeoutMs_;
    bool active_=false;
    AIError abortReason_=AIError::None;
};
}
