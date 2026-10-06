#pragma once
#include <QObject>
#include <QElapsedTimer>
#include "core/GenerationContext.h"
#include "ai/IAIProvider.h"
#include "platform/ISecretStore.h"
#include "platform/ISelectionProvider.h"
namespace ws {
struct RequestMetrics {QString provider,model,feature,outcome="none";qint64 durationMs=0;};
class AppController : public QObject {
 Q_OBJECT
public:
    AppController(IAIProvider&,ISecretStore&,ISelectionProvider&,QSettings&,QObject* parent=nullptr);
    bool captureClipboard();
    bool run(Feature);
    bool runSelection(Feature,const Selection&);
    bool generateReply(const Selection&,const QString& stance,const QString& intent);
    bool refine(const QString& adjustment);
    void clearGeneration();
    const std::optional<GenerationContext>& generationContext() const {return context_;}
    const RequestMetrics& metrics() const {return metrics_;}
    bool testConnection();
    void cancel();
    bool busy() const;
signals:
    void replyRequested(const ws::Selection&);
    void captured(const QString&);
    void loading();
    void finished(const ws::AIResult&);
private:
    bool generate(Feature,const Selection&,const QString& variant={},const QString& intent={});
    bool startRequest(const Prompt&,const Settings&);
    void complete(const AIResult&);
    IAIProvider& ai_;
    ISecretStore& secrets_;
    ISelectionProvider& selection_;
    QSettings& settings_;
    std::optional<Selection> captured_;
    std::optional<GenerationContext> context_;
    AIRequest pendingRequest_;
    bool awaitingSecret_=false;
    bool acceptCompletion_=false;
    QElapsedTimer timer_;
    RequestMetrics metrics_;
};
}
