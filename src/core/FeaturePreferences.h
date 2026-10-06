#pragma once
#include "Core.h"
#include <QStringList>
namespace ws {
struct FeaturePreferences {
    QStringList enabled,quickActions;
    static FeaturePreferences load(QSettings&);
    bool save(QSettings&) const;
};
}
