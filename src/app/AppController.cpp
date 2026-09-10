#include "AppController.h"
namespace ws {
AppController::AppController(IAIProvider& ai, ISecretStore& secrets, ISelectionProvider& selection,
                             QSettings& settings, QObject* parent)
    : QObject(parent), ai_(ai), secrets_(secrets), selection_(selection), settings_(settings) {
    connect(&secrets_, &ISecretStore::readFinished, this, [this](const SecretResult& result) {
        if (!awaitingSecret_) return;
        awaitingSecret_ = false;
        if (!result.error.isEmpty()) {
            pendingRequest_ = {};
            emit finished({{}, AIError::MissingKey, result.error});
            return;
        }
        pendingRequest_.apiKey = result.key;
        const bool accepted = ai_.generate(pendingRequest_);
        pendingRequest_ = {};
        if (!accepted) emit finished({{}, AIError::Network, "Another AI request is in progress."});
    });
    connect(&ai_, &IAIProvider::completed, this, &AppController::finished);
}
bool AppController::busy() const { return awaitingSecret_ || ai_.busy(); }
bool AppController::captureClipboard() {
    if (busy()) return false;
    captured_ = selection_.currentSelection();
    emit captured(captured_ ? captured_->text : QString{});
    if (!captured_) emit finished({{}, AIError::EmptyResponse, "No copied text. Copy text and try again."});
    return captured_.has_value();
}
bool AppController::run(Feature feature) {
    if (busy() || secrets_.busy()) return false;
    if (!captured_) {
        emit finished({{}, AIError::EmptyResponse, "No copied text. Choose Process Clipboard first."});
        return false;
    }
    return runText(feature, captured_->text);
}
bool AppController::testConnection() { return runText(Feature::PlainSpeak, "This is a connection test. Reply with OK."); }
bool AppController::runText(Feature feature, const QString& text) {
    if (busy() || secrets_.busy()) return false;
    const auto settings = Settings::load(settings_);
    const auto prompt = PromptBuilder::build({feature, text, settings.outputLanguage, {}});
    if (!prompt.error.isEmpty()) {
        emit finished({{}, AIError::EmptyResponse, prompt.error}); return false;
    }
    if (settings.model.trimmed().isEmpty()) {
        emit finished({{}, AIError::MissingModel, "Enter a model in Settings first."}); return false;
    }
    pendingRequest_ = {prompt, settings.model, {}};
    awaitingSecret_ = true;
    emit loading();
    if (!secrets_.read()) {
        awaitingSecret_ = false; pendingRequest_ = {};
        emit finished({{}, AIError::MissingKey, "Secret store is busy. Please retry."});
        return false;
    }
    return true;
}
void AppController::cancel() {
    if (awaitingSecret_) {
        awaitingSecret_ = false; pendingRequest_ = {};
        emit finished({{}, AIError::Cancelled, "Request cancelled."});
    }
    ai_.cancel();
}
}
