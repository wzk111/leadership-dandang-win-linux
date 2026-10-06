#pragma once
#include <QString>
#include <QSettings>
namespace ws {
struct Profile {
    QString displayName,role,team,responsibilities,currentProjects,communicationPreferences,additionalContext;
    bool valid() const;
    QString serialize() const;
    static Profile load(QSettings&);
    bool save(QSettings&) const;
    static void clear(QSettings&);
};
}
