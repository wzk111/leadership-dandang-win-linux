#include "../PlatformFactory.h"
#include "LinuxSecretStore.h"
#include "AtSpiSelectionMonitor.h"
namespace ws {
void preparePlatformEventLoop() { qputenv("QT_NO_GLIB", "1"); }
std::unique_ptr<ISelectionMonitor> createSelectionMonitor() {
    return std::make_unique<AtSpiSelectionMonitor>(createAtSpiSession());
}
std::unique_ptr<ISecretStore> createSecretStore() { return std::make_unique<LinuxSecretStore>(); }
}
