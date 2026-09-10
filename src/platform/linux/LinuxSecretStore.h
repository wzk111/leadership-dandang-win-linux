#pragma once
#include "../ISecretStore.h"
#include <QFutureWatcher>
namespace ws {
class LinuxSecretStore : public ISecretStore {
    Q_OBJECT
public:
    explicit LinuxSecretStore(QObject* parent = nullptr, QString account = "openai");
    bool read() override;
    bool save(const QString& key) override;
    bool remove() override;
    bool busy() const override;
    QString description() const override;
private:
    bool start(int operation, const QString& key = {});
    bool active_ = false;
    QString account_;
    QFutureWatcher<SecretResult> watcher_;
};
}
