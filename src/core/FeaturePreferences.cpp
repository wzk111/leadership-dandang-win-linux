#include "FeaturePreferences.h"
namespace ws {
FeaturePreferences FeaturePreferences::load(QSettings&) {return {};}
bool FeaturePreferences::save(QSettings&) const {return false;}
}
