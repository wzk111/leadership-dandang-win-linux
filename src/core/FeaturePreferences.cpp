#include "FeaturePreferences.h"
namespace ws {
static FeaturePreferences normalize(FeaturePreferences p) {
    QStringList enabled,quick;
    for(const auto& id:p.enabled)if(FeatureRegistry::fromId(id) && !enabled.contains(id))enabled<<id;
    for(const auto& id:p.quickActions)if(enabled.contains(id) && !quick.contains(id) && quick.size()<5)quick<<id;
    for(const auto& id:enabled)if(quick.size()<qMin(3,enabled.size()) && !quick.contains(id))quick<<id;
    return {enabled,quick};
}
FeaturePreferences FeaturePreferences::load(QSettings& s) {
    QStringList all;for(const auto& f:FeatureRegistry::all())all<<f.stableId;
    return normalize({s.value("features/enabled",all).toStringList(),
        s.value("features/quickActions",QStringList{"plain_speak","summarize","polish","reply"}).toStringList()});
}
bool FeaturePreferences::save(QSettings& s) const {
    const auto p=normalize(*this);s.setValue("features/enabled",p.enabled);s.setValue("features/quickActions",p.quickActions);
    s.sync();return s.status()==QSettings::NoError;
}
}
