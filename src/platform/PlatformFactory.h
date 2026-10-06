#pragma once
#include <memory>
#include "ISecretStore.h"
#include "ISelectionMonitor.h"
#include "IPlatformWindowPolicy.h"
#include "IGlobalShortcut.h"
#include "core/SelectionResolver.h"
class QClipboard;
namespace ws {
std::unique_ptr<ISecretStore> createSecretStore();
std::unique_ptr<IGlobalShortcut> createGlobalShortcut();
std::unique_ptr<IExplicitSelectionResolver> createExplicitResolver(ISelectionMonitor* monitor, QClipboard& clipboard);
void preparePlatformEventLoop();
std::unique_ptr<ISelectionMonitor> createSelectionMonitor();
std::unique_ptr<IPlatformWindowPolicy> createWindowPolicy();
}
