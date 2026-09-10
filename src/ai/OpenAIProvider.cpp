#include "OpenAIProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
namespace ws {
namespace {
AIResult failure(AIError error) {
    QString message;
    switch (error) {
    case AIError::MissingKey: message = "API key missing. Save a key in Settings."; break;
    case AIError::MissingModel: message = "Model missing. Enter a Responses-compatible model in Settings."; break;
    case AIError::Authentication: message = "Authentication failed. Check your API key and project permissions."; break;
    case AIError::RateLimited: message = "Rate limited or API quota exhausted. Check billing and try again later."; break;
    case AIError::Timeout: message = "Request timed out. Please try again."; break;
    case AIError::Cancelled: message = "Request cancelled."; break;
    case AIError::InvalidJson: message = "The API returned an invalid JSON response."; break;
    case AIError::EmptyResponse: message = "The API returned no text."; break;
    case AIError::Refused: message = "The AI provider declined this request."; break;
    case AIError::Incomplete: message = "The response was incomplete. Try a shorter selection."; break;
    case AIError::TooLarge: message = "The API response exceeded the size limit."; break;
    case AIError::Server: message = "The API rejected the request or is unavailable. Check the model and try again."; break;
    default: message = "Network unavailable. Check your connection and HTTPS access."; break;
    }
    return {{}, error, message};
}
}
OpenAIProvider::OpenAIProvider(QObject* parent, QUrl endpoint, int timeoutMs)
    : IAIProvider(parent), endpoint_(std::move(endpoint)), timeoutMs_(timeoutMs) {
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, [this] {
        if (reply_) { abortReason_ = AIError::Timeout; reply_->abort(); }
    });
}
bool OpenAIProvider::busy() const { return !reply_.isNull(); }
bool OpenAIProvider::generate(const AIRequest& r) {
    if (busy()) return false;
    if (r.apiKey.trimmed().isEmpty() || r.model.trimmed().isEmpty()) {
        const auto result = failure(r.apiKey.trimmed().isEmpty() ? AIError::MissingKey : AIError::MissingModel);
        QTimer::singleShot(0, this, [this, result] { emit completed(result); });
        return true;
    }
    // Plain HTTP is permitted only for injected loopback test servers.
    if (endpoint_.scheme() != "https" &&
        !(endpoint_.scheme() == "http" && endpoint_.host() == "127.0.0.1")) {
        QTimer::singleShot(0, this, [this] { emit completed(failure(AIError::Network)); });
        return true;
    }
    QNetworkRequest request(endpoint_);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + r.apiKey.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    const QJsonObject payload{{"model", r.model.trimmed()}, {"instructions", r.prompt.system},
                              {"input", r.prompt.user}, {"store", false}};
    body_.clear();
    abortReason_ = AIError::None;
    reply_ = network_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    auto* reply = reply_.data();
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        body_ += reply->readAll();
        if (body_.size() > 4 * 1024 * 1024) {
            abortReason_ = AIError::TooLarge;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        timer_.stop();
        if (reply->isOpen()) body_ += reply->readAll();
        const auto result = abortReason_ != AIError::None ? failure(abortReason_)
            : body_.size() > 4 * 1024 * 1024 ? failure(AIError::TooLarge)
            : decode(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->error(), body_);
        reply_ = nullptr;
        body_.clear();
        reply->deleteLater();
        emit completed(result);
    });
    timer_.start(timeoutMs_);
    return true;
}
void OpenAIProvider::cancel() {
    if (reply_) { abortReason_ = AIError::Cancelled; reply_->abort(); }
}
AIResult OpenAIProvider::decode(int status, QNetworkReply::NetworkError networkError, const QByteArray& body) {
    if (status == 401 || status == 403) return failure(AIError::Authentication);
    if (status == 429) return failure(AIError::RateLimited);
    if (status >= 300) return failure(AIError::Server);
    if (networkError != QNetworkReply::NoError || status < 200) return failure(AIError::Network);
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) return failure(AIError::InvalidJson);
    const auto object = doc.object();
    if (object["status"].toString() == "incomplete") return failure(AIError::Incomplete);
    if (object["status"].toString() == "failed" || object["error"].isObject()) return failure(AIError::Server);
    if (object["status"].toString() != "completed") return failure(AIError::Incomplete);
    QStringList pieces;
    for (const auto& item : object["output"].toArray()) {
        const auto message = item.toObject();
        if (message["type"].toString() != "message") continue;
        for (const auto& content : message["content"].toArray()) {
            const auto part = content.toObject();
            if (part["type"].toString() == "refusal") return failure(AIError::Refused);
            if (part["type"].toString() == "output_text") pieces.append(part["text"].toString());
        }
    }
    const auto text = pieces.join("\n").trimmed();
    return text.isEmpty() ? failure(AIError::EmptyResponse) : AIResult{text, AIError::None, {}};
}
}
