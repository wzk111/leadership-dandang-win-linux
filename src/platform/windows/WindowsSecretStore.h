#pragma once
#include "../ISecretStore.h"
#include <QFutureWatcher>
namespace ws {
class WindowsSecretStore:public ISecretStore {
 Q_OBJECT
public:
    explicit WindowsSecretStore(QObject* parent=nullptr,QString account="openai",QString targetPrefix="WorkSidekick/");
    bool read()override;
    bool save(const QString&)override;
    bool remove()override;
    bool busy()const override;
    QString description()const override;
private:
    bool start(int,const QString& key={});
    QString target_;
    bool active_=false;
    QFutureWatcher<SecretResult> watcher_;
};
}
