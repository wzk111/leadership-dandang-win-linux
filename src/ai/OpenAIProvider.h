#pragma once
#include "IAIProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <QUrl>
namespace ws {
class OpenAIProvider : public IAIProvider {
    Q_OBJECT
public:
    explicit OpenAIProvider(QObject* parent = nullptr,
                            QUrl endpoint = QUrl("https://api.openai.com/v1/responses"),
                            int timeoutMs = 30000);
    bool generate(const AIRequest& request) override;
    void cancel() override;
    bool busy() const override;
    static AIResult decode(int status, QNetworkReply::NetworkError networkError, const QByteArray& body);
private:
    void finishLater(AIError error);
    bool active_ = false;
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QTimer timer_;
    QUrl endpoint_;
    QByteArray body_;
    int timeoutMs_;
    AIError abortReason_ = AIError::None;
};
}
