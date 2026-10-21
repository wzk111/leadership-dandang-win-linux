#include "GeminiProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkRequest>
#include <algorithm>
namespace ws {
GeminiProvider::GeminiProvider(QObject* p,QUrl e,int t):HttpAIProvider("Gemini",e,t,p){}
QJsonObject GeminiProvider::payload(const AIRequest& r)const {
    return {{"model",r.model.trimmed()},{"system_instruction",r.prompt.system},{"input",r.prompt.user},
        {"store",false},{"generation_config",QJsonObject{{"max_output_tokens",std::clamp(r.maxOutputTokens,1,16384)}}}};
}
void GeminiProvider::authorize(QNetworkRequest& request,const QString& key)const {
    request.setRawHeader("x-goog-api-key",key.toUtf8());
}
AIResult GeminiProvider::parse(int status,QNetworkReply::NetworkError error,const QByteArray& body)const {
    const auto transport=transportError(status,error);
    if(transport!=AIError::None)return failure(transport,provider_);
    QJsonParseError jsonError;
    const auto document=QJsonDocument::fromJson(body,&jsonError);
    if(jsonError.error!=QJsonParseError::NoError || !document.isObject())return failure(AIError::InvalidJson,provider_);
    const auto object=document.object();
    if(object["error"].isObject() || object["status"].toString()=="failed")return failure(AIError::Server,provider_);
    if(object["status"].toString()=="blocked" || object["status"].toString()=="refused")return failure(AIError::Refused,provider_);
    if(object["status"].toString()!="completed")return failure(AIError::Incomplete,provider_);
    if(!object["steps"].isArray())return failure(AIError::InvalidJson,provider_);
    QStringList pieces;
    for(const auto& value:object["steps"].toArray()) {
        const auto step=value.toObject();
        if(step["type"].toString()!="model_output")continue;
        for(const auto& content:step["content"].toArray()) {
            const auto part=content.toObject();
            if(part["type"].toString()=="refusal")return failure(AIError::Refused,provider_);
            if(part["type"].toString()=="text")pieces<<part["text"].toString();
        }
    }
    const auto text=pieces.join("\n").trimmed();
    return text.isEmpty()?failure(AIError::EmptyResponse,provider_):AIResult{text,AIError::None,{}};
}
}
