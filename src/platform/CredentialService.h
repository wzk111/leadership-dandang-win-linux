#pragma once
#include "ISecretStore.h"
#include <QMap>
namespace ws {
class CredentialService : public ISecretStore {
 Q_OBJECT
public:
    // Adopts the provider-specific stores; all operations remain asynchronous.
    explicit CredentialService(QMap<QString,ISecretStore*> stores,QObject* parent=nullptr);
    bool selectProvider(const QString&)override;
    bool read()override;
    bool save(const QString&)override;
    bool remove()override;
    bool busy()const override;
    QString description()const override;
    QString credentialStatus(const QString&)const override;
private:
    bool start(int operation,const QString& key={});
    QMap<QString,ISecretStore*> stores_;
    QMap<QString,QString> status_;
    QString selected_="openai";
    bool active_=false;
    int operation_=-1;
};
}
