#include "AppController.h"
#include "core/Profile.h"
namespace ws {
AppController::AppController(IAIProvider& ai,ISecretStore& secrets,ISelectionProvider& selection,QSettings& settings,QObject* parent)
    :QObject(parent),ai_(ai),secrets_(secrets),selection_(selection),settings_(settings) {
    connect(&secrets_,&ISecretStore::readFinished,this,[this](const SecretResult& result) {
        if(!awaitingSecret_)return;
        awaitingSecret_=false;
        if(!result.error.isEmpty()) {pendingRequest_={};complete({{},AIError::MissingKey,"Secret store unavailable. Unlock your keyring and retry."});return;}
        pendingRequest_.apiKey=result.key;
        const bool accepted=ai_.generate(pendingRequest_);pendingRequest_={};
        if(!accepted)complete({{},AIError::Network,"Another AI request is in progress."});
    });
    connect(&ai_,&IAIProvider::completed,this,&AppController::complete);
}
bool AppController::busy() const {return awaitingSecret_ || ai_.busy();}
bool AppController::captureClipboard() {
    if(busy())return false;
    captured_=selection_.currentSelection();emit captured(captured_?captured_->text:QString{});
    if(!captured_)emit finished({{},AIError::EmptyResponse,"No copied text. Copy text and try again."});
    return captured_.has_value();
}
bool AppController::run(Feature feature) {
    if(busy())return false;
    if(!captured_) {emit finished({{},AIError::EmptyResponse,"No copied text. Choose Process Clipboard first."});return false;}
    return runSelection(feature,*captured_);
}
bool AppController::runSelection(Feature feature,const Selection& selection) {
    if(busy() || selection.text.trimmed().isEmpty() || selection.text.size()>100000)return false;
    if(FeatureRegistry::info(feature).requiresComposer) {emit replyRequested(selection);return true;}
    return generate(feature,selection,feature==Feature::Polish?settings_.value("features/polishVariant","Default").toString():QString{});
}
bool AppController::generateReply(const Selection& selection,const QString& stance,const QString& intent) {
    return generate(Feature::Reply,selection,stance,intent);
}
bool AppController::generate(Feature feature,const Selection& selection,const QString& variant,const QString& intent) {
    if(busy())return false;
    const auto config=Settings::load(settings_);
    const bool useProfile=FeatureRegistry::info(feature).requiresProfile && settings_.value("profile/enabled",false).toBool();
    PromptRequest request{feature,selection.text,config.outputLanguage,useProfile?Profile::load(settings_).serialize():QString{}};
    request.variant=variant;request.userIntent=intent;
    context_=GenerationContext{request,config,{}};
    return startRequest(PromptBuilder::build(request),config);
}
bool AppController::startRequest(const Prompt& prompt,const Settings& config) {
    if(busy())return false;
    metrics_={config.providerId,config.model,context_?FeatureRegistry::info(context_->request.feature).stableId:QString("test"),"pending",0};
    timer_.start();acceptCompletion_=true;
    if(!prompt.error.isEmpty()) {complete({{},AIError::EmptyResponse,prompt.error});return false;}
    if(config.model.trimmed().isEmpty()) {complete({{},AIError::MissingModel,"Enter a model in Settings first."});return false;}
    if(secrets_.busy() || !secrets_.selectProvider(config.providerId)) {
        complete({{},AIError::MissingKey,"Secret store is busy or unavailable. Wait and retry."});return false;
    }
    pendingRequest_={prompt,config.model,{},config.maxOutputTokens,config.providerId,config.baseUrl};
    awaitingSecret_=true;emit loading();
    if(!secrets_.read()) {
        awaitingSecret_=false;pendingRequest_={};complete({{},AIError::MissingKey,"Secret store is busy. Please retry."});return false;
    }
    return true;
}
void AppController::complete(const AIResult& result) {
    if(!acceptCompletion_)return;
    acceptCompletion_=false;
    metrics_.durationMs=timer_.isValid()?timer_.elapsed():0;
    metrics_.outcome=result.error==AIError::None?"success":QString("error %1").arg(int(result.error));
    if(context_ && result.error==AIError::None)context_->lastResult=result.text;
    emit finished(result);
}
bool AppController::refine(const QString& adjustment) {
    if(busy() || !context_ || context_->lastResult.isEmpty())return false;
    const QStringList allowed{"Regenerate","Shorter","Longer","Friendlier","More Direct"};
    if(!allowed.contains(adjustment))return false;
    auto request=context_->request;
    if(adjustment!="Regenerate") {request.refinementInstruction=adjustment;request.previousResult=context_->lastResult;}
    return startRequest(PromptBuilder::build(request),context_->config);
}
bool AppController::testConnection() {
    if(busy())return false;
    context_.reset();
    return startRequest({"Return only OK. This is a connectivity test.","Reply only with OK.",{}},Settings::load(settings_));
}
void AppController::cancel() {
    if(awaitingSecret_) {
        awaitingSecret_=false;pendingRequest_={};complete({{},AIError::Cancelled,"Request cancelled."});
    }
    if(ai_.busy())ai_.cancel();
}
void AppController::clearGeneration() {
    acceptCompletion_=false;context_.reset();cancel();
}
}
