#include "Profile.h"
#include <array>
namespace ws {
namespace {
struct Field {const char* key;const char* label;QString Profile::* value;int limit;};
const std::array<Field,7> fields{{
    {"displayName","Name",&Profile::displayName,100},{"role","Role",&Profile::role,200},
    {"team","Team",&Profile::team,200},{"responsibilities","Responsibilities",&Profile::responsibilities,2000},
    {"currentProjects","Current projects",&Profile::currentProjects,2000},
    {"communicationPreferences","Communication preferences",&Profile::communicationPreferences,1000},
    {"additionalContext","Additional context",&Profile::additionalContext,2000}
}};
}
bool Profile::valid() const {
    for(const auto& f:fields)if((this->*f.value).size()>f.limit)return false;
    return true;
}
QString Profile::serialize() const {
    if(!valid())return {};
    QStringList lines;
    for(const auto& f:fields) {
        const auto value=(this->*f.value).trimmed();
        if(!value.isEmpty())lines<<QString(f.label)+": "+value;
    }
    const auto text=lines.join("\n");
    return text.size()<=8000?text:QString{};
}
Profile Profile::load(QSettings& s) {
    Profile p;
    for(const auto& f:fields)p.*f.value=s.value(QString("profile/")+f.key).toString();
    return p.valid()?p:Profile{};
}
bool Profile::save(QSettings& s) const {
    if(!valid())return false;
    for(const auto& f:fields)s.setValue(QString("profile/")+f.key,this->*f.value);
    s.sync();return s.status()==QSettings::NoError;
}
void Profile::clear(QSettings& s) {s.remove("profile");s.setValue("profile/enabled",false);s.sync();}
}
