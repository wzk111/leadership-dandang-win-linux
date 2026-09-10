#include "LinuxSecretStore.h"
namespace ws {
LinuxSecretStore::LinuxSecretStore(QObject* parent, QString account) : ISecretStore(parent), account_(account) {}
bool LinuxSecretStore::read() { return false; }
bool LinuxSecretStore::save(const QString&) { return false; }
bool LinuxSecretStore::remove() { return false; }
bool LinuxSecretStore::busy() const { return false; }
QString LinuxSecretStore::description() const { return {}; }
bool LinuxSecretStore::start(int, const QString&) { return false; }
}
