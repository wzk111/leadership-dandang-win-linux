#pragma once
#include <QObject>
namespace ws {
class PortalTransport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void probe()=0;
    virtual void create()=0;
    virtual void bind(const QString& session)=0;
    virtual void close()=0;
signals:
    void probed(uint version, const QString& error);
    void created(const QString& session, const QString& error);
    void bound(bool registered, const QString& trigger, const QString& error);
    void activation(const QString& session, const QString& id);
    void lost();
};
}
