#include "HttpAIProvider.h"
#include <QJsonDocument>
#include <QNetworkRequest>
namespace ws {
namespace { constexpr qint64 responseLimit=4*1024*1024; }
HttpAIProvider::HttpAIProvider(QString id,QUrl endpoint,int timeoutMs,QObject* parent)
    :IAIProvider(parent),provider_(std::move(id)),endpoint_(std::move(endpoint)),timeoutMs_(timeoutMs) {
    timer_.setSingleShot(true);
    connect(&timer_,&QTimer::timeout,this,[this] {
        if(reply_) {abortReason_=AIError::Timeout;reply_->abort();}
    });
}
bool HttpAIProvider::busy()const{return active_;}
void HttpAIProvider::finishLater(AIError error) {
    abortReason_=error;
    QTimer::singleShot(0,this,[this] {
        const auto result=failure(abortReason_,provider_);
        active_=false;
        emit completed(result);
    });
}
bool HttpAIProvider::generate(const AIRequest& r) {
    if(active_)return false;
    active_=true;
    if(r.apiKey.trimmed().isEmpty() || r.model.trimmed().isEmpty()) {
        finishLater(r.apiKey.trimmed().isEmpty()?AIError::MissingKey:AIError::MissingModel);return true;
    }
    const auto endpoint=endpointFor(r);
    // HTTP endpoints are possible only through constructor injection or the compatible test flag.
    if(!endpoint.isValid() || endpoint.host().isEmpty() || !endpoint.userInfo().isEmpty() ||
       endpoint.hasQuery() || endpoint.hasFragment() ||
       (endpoint.scheme()!="https" && !(endpoint.scheme()=="http" && endpoint.host()=="127.0.0.1"))) {
        finishLater(AIError::Network);return true;
    }
    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    authorize(request,r.apiKey);
    body_.clear();abortReason_=AIError::None;
    reply_=network_.post(request,QJsonDocument(payload(r)).toJson(QJsonDocument::Compact));
    auto* reply=reply_.data();
    reply->setReadBufferSize(responseLimit+1);
    connect(reply,&QNetworkReply::readyRead,this,[this,reply]{readAvailable(reply);});
    connect(reply,&QNetworkReply::finished,this,[this,reply] {
        timer_.stop();
        if(reply->isOpen())readAvailable(reply);
        const auto result=abortReason_!=AIError::None?failure(abortReason_,provider_):
            parse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),reply->error(),body_);
        reply_=nullptr;active_=false;body_.clear();reply->deleteLater();
        emit completed(result);
    });
    timer_.start(timeoutMs_);
    return true;
}
void HttpAIProvider::readAvailable(QNetworkReply* reply) {
    if(abortReason_!=AIError::None)return;
    body_+=reply->read(responseLimit+1-body_.size());
    if(body_.size()>responseLimit) {
        abortReason_=AIError::TooLarge;
        if(!reply->isFinished())reply->abort();
    }
}
void HttpAIProvider::cancel() {
    if(!active_)return;
    abortReason_=AIError::Cancelled;
    if(reply_)reply_->abort();
}
AIError HttpAIProvider::transportError(int status,QNetworkReply::NetworkError error) {
    if(status==401 || status==403)return AIError::Authentication;
    if(status==429)return AIError::RateLimited;
    if(status>=300)return AIError::Server;
    if(error!=QNetworkReply::NoError || status<200)return AIError::Network;
    return AIError::None;
}
AIResult HttpAIProvider::failure(AIError error,const QString& provider) {
    QString message;
    switch(error) {
    case AIError::MissingKey:message="API key missing. Save a key in Settings.";break;
    case AIError::MissingModel:message="Model missing. Enter a model in Settings.";break;
    case AIError::Authentication:message="Authentication failed. Check your API key and permissions.";break;
    case AIError::RateLimited:message="Rate limited or quota exhausted. Try again later.";break;
    case AIError::Timeout:message="Request timed out. Please try again.";break;
    case AIError::Cancelled:message="Request cancelled.";break;
    case AIError::InvalidJson:message="The API returned an invalid response.";break;
    case AIError::EmptyResponse:message="The API returned no text.";break;
    case AIError::Refused:message="The AI provider declined this request.";break;
    case AIError::Incomplete:message="The response was incomplete. Try a shorter selection.";break;
    case AIError::TooLarge:message="The API response exceeded the size limit.";break;
    case AIError::Server:message="The API rejected the request or is unavailable. Check the model.";break;
    default:message="Network unavailable. Check your connection and HTTPS endpoint.";break;
    }
    return {{},error,provider.isEmpty()?message:provider+": "+message};
}
QUrl HttpAIProvider::endpointFor(const AIRequest&)const{return endpoint_;}
}
