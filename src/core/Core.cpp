#include "Core.h"
namespace ws {
QList<FeatureInfo> FeatureRegistry::all() { return {}; }
Prompt PromptBuilder::build(const PromptRequest&) { return {}; }
Settings Settings::load(QSettings&) { return {}; }
bool Settings::save(QSettings&) const { return false; }
}
