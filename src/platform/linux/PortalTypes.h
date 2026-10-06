#pragma once
#include <QDBusArgument>
#include <QVariantMap>
namespace ws {
struct PortalBinding { QString id; QVariantMap options; };
using PortalBindings=QList<PortalBinding>;
inline QDBusArgument& operator<<(QDBusArgument& a,const PortalBinding& b) {
    a.beginStructure(); a<<b.id<<b.options; a.endStructure(); return a;
}
inline const QDBusArgument& operator>>(const QDBusArgument& a,PortalBinding& b) {
    a.beginStructure(); a>>b.id>>b.options; a.endStructure(); return a;
}
}
Q_DECLARE_METATYPE(ws::PortalBinding)
Q_DECLARE_METATYPE(ws::PortalBindings)
