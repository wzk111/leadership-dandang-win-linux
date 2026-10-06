#include "Profile.h"
namespace ws {
bool Profile::valid() const { return false; }
QString Profile::serialize() const {return {};}
Profile Profile::load(QSettings&) {return {};}
bool Profile::save(QSettings&) const {return false;}
void Profile::clear(QSettings&) {}
}
