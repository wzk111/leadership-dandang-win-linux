#pragma once
#include <memory>
#include "ISecretStore.h"
#include "ISelectionMonitor.h"
#include "IPlatformWindowPolicy.h"
namespace ws {
std::unique_ptr<ISecretStore> createSecretStore();
void preparePlatformEventLoop();
std::unique_ptr<ISelectionMonitor> createSelectionMonitor();
std::unique_ptr<IPlatformWindowPolicy> createWindowPolicy();
}
