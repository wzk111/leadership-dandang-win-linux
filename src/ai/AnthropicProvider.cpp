#include "AnthropicProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkRequest>
#include <algorithm>
namespace ws {
AnthropicProvider::AnthropicProvider(QObject* p,QUrl e,int t):HttpAIProvider("Anthropic",e,t,p){}
QJsonObject AnthropicProvider::payload(const AIRequest& r)const {
    return {{"model",r.model.trimmed()},{"system",r.prompt.system},
        {"messages",QJsonArray{QJsonObject{{"role","user"},{"content",r.prompt.user}}}},
        {"max_tokens",std::clamp(r.maxOutputTokens,1,16384)}};
}
void AnthropicProvider::authorize(QNetworkRequest& request,const QString& key)const {
    request.setRawHeader("x-api-key",key.toUtf8());
    request.setRawHeader("anthropic-version","2023-06-01");
}
AIResult AnthropicProvider::parse(int status,QNetworkReply::NetworkError error,const QByteArray& body)const {
    const auto transport=transportError(status,error);
    if(transport!=AIError::None)return failure(transport,provider_);
    QJsonParseError jsonError;
    const auto document=QJsonDocument::fromJson(body,&jsonError);
    if(jsonError.error!=QJsonParseError::NoError || !document.isObject())return failure(AIError::InvalidJson,provider_);
    const auto object=document.object();
    if(object["type"].toString()=="error" || object["error"].isObject())return failure(AIError::Server,provider_);
    if(object["type"].toString()!="message" || object["role"].toString()!="assistant" || !object["content"].isArray())return failure(AIError::InvalidJson,provider_);
    const auto stop=object["stop_reason"].toString();
    if(stop=="refusal")return failure(AIError::Refused,provider_);
    if(stop!="end_turn" && stop!="stop_sequence")return failure(AIError::Incomplete,provider_);
    QStringList pieces;
    for(const auto& value:object["content"].toArray()) {
        const auto part=value.toObject();
        if(part["type"].toString()=="text")pieces<<part["text"].toString();
    }
    const auto text=pieces.join("\n").trimmed();
    return text.isEmpty()?failure(AIError::EmptyResponse,provider_):AIResult{text,AIError::None,{}};
}
}
