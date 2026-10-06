#pragma once
#include <QObject>
#include "core/GenerationContext.h"
#include "ai/IAIProvider.h"
#include "platform/ISecretStore.h"
#include "platform/ISelectionProvider.h"
namespace ws {
class AppController : public QObject {
    Q_OBJECT
public:
    AppController(IAIProvider& ai, ISecretStore& secrets, ISelectionProvider& selection,
                  QSettings& settings, QObject* parent = nullptr);
    bool captureClipboard();
    bool run(Feature feature);
    bool runSelection(Feature feature, const Selection& selection);
    bool generateReply(const Selection&, const QString&, const QString&);
    bool refine(const QString&);
    void clearGeneration();
    const std::optional<GenerationContext>& generationContext() const {return context_;}
    bool testConnection();
    void cancel();
    bool busy() const;
signals:
    void replyRequested(const ws::Selection& selection);
    void captured(const QString& text);
    void loading();
    void finished(const ws::AIResult& result);
private:
    bool runText(Feature feature, const QString& text);
    IAIProvider& ai_;
    ISecretStore& secrets_;
    ISelectionProvider& selection_;
    QSettings& settings_;
    std::optional<Selection> captured_;
    AIRequest pendingRequest_;
    bool awaitingSecret_ = false;
    std::optional<GenerationContext> context_;
};
}
