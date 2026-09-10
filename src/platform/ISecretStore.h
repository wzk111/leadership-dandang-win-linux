#pragma once
#include <QObject>
#include <QString>
namespace ws {
struct SecretResult { QString key; QString error; };
class ISecretStore : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual bool read() = 0;
    virtual bool save(const QString& key) = 0;
    virtual bool remove() = 0;
    virtual bool busy() const = 0;
    virtual QString description() const = 0;
signals:
    void readFinished(const ws::SecretResult& result);
    void writeFinished(bool success, const QString& message);
};
}
Q_DECLARE_METATYPE(ws::SecretResult)
