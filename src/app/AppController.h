#pragma once
#include <QObject>
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
    bool testConnection();
    void cancel();
    bool busy() const;
signals:
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
};
}
