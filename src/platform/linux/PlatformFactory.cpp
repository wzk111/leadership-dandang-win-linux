#include "../PlatformFactory.h"
#include "LinuxSecretStore.h"
namespace ws {
std::unique_ptr<ISecretStore> createSecretStore() { return std::make_unique<LinuxSecretStore>(); }
}
