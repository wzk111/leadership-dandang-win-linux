#include "AIService.h"
#include "OpenAIProvider.h"
#include "AnthropicProvider.h"
#include "GeminiProvider.h"
#include "OpenAICompatibleProvider.h"
#include <QTimer>
#include <QSet>
namespace ws {
AIService::AIService(QObject* p,QMap<QString,IAIProvider*> providers)
    :IAIProvider(p),providers_(std::move(providers)) {
    if(providers_.isEmpty()) {
        providers_={{"openai",new OpenAIProvider(this)},{"anthropic",new AnthropicProvider(this)},
            {"gemini",new GeminiProvider(this)},{"openai_compatible",new OpenAICompatibleProvider(this)}};
    }
    QSet<IAIProvider*> connected;
    for(auto* provider:providers_) {
        if(!provider || connected.contains(provider))continue;
        connected.insert(provider);
        provider->setParent(this);
        connect(provider,&IAIProvider::completed,this,[this,provider](const AIResult& result) {
            if(active_!=provider)return;
            active_=nullptr;
            emit completed(result);
        });
    }
}
bool AIService::generate(const AIRequest& request) {
    if(busy())return false;
    auto* provider=providers_.value(request.providerId,nullptr);
    if(!provider) {
        pendingError_=true;pendingCancelled_=false;
        QTimer::singleShot(0,this,[this] {
            const auto result=HttpAIProvider::failure(pendingCancelled_?AIError::Cancelled:AIError::Server,{});
            pendingError_=false;pendingCancelled_=false;
            emit completed(result);
        });
        return true;
    }
    active_=provider;
    if(!provider->generate(request)) {active_=nullptr;return false;}
    return true;
}
void AIService::cancel() {
    if(pendingError_)pendingCancelled_=true;
    else if(active_)active_->cancel();
}
bool AIService::busy()const{return active_ || pendingError_;}
}
