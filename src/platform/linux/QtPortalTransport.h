#pragma once
#include "PortalTransport.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QTimer>
namespace ws {
class QtPortalTransport : public PortalTransport {
    Q_OBJECT
public:
    explicit QtPortalTransport(QString service="org.freedesktop.portal.Desktop");
    ~QtPortalTransport() override;
    void probe() override;
    void create() override;
    void bind(const QString& session) override;
    void close() override;
private slots:
    void response(uint code, const QVariantMap& results, const QDBusMessage& message);
    void activated(const QDBusObjectPath& session, const QString& id, qulonglong timestamp, const QVariantMap& options);
    void sessionClosed(const QVariantMap& details);
    void disconnected();
private:
    void request(const QString& method, QVariantList arguments, QVariantMap options);
    void finishError(const QString& message);
    void closePath(const QString& path,const QString& interface);
    QString service_, request_, session_;
    QDBusConnection bus_;
    QTimer timeout_;
    enum class Pending { Idle, Create, Bind };
    Pending pending_=Pending::Idle;
    quint64 generation_=0;
};
}
