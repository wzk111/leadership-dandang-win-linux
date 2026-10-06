#pragma once
#include <QObject>
#include <QDateTime>
namespace ws {
inline const QString ShortcutId = QStringLiteral("worksidekick.trigger");
inline const QString PreferredShortcut = QStringLiteral("Ctrl+Alt+P");
struct GlobalShortcutStatus {
    bool available=false;
    bool registered=false;
    QString backend="Unavailable";
    QString description="Disabled";
    QString triggerDescription;
    uint portalVersion=0;
    QDateTime lastActivation;
};
class IGlobalShortcut : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void start(bool explicitEnable) = 0;
    virtual void stop() = 0;
    virtual GlobalShortcutStatus status() const = 0;
signals:
    void activated();
    void statusChanged();
};
}
Q_DECLARE_METATYPE(ws::GlobalShortcutStatus)
