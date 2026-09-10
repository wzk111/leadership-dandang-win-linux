#pragma once
#include <memory>
#include "ISecretStore.h"
#include "ISelectionMonitor.h"
namespace ws {
std::unique_ptr<ISecretStore> createSecretStore();
void preparePlatformEventLoop();
std::unique_ptr<ISelectionMonitor> createSelectionMonitor();
}
