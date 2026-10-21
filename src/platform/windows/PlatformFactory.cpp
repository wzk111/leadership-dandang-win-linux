#include "../PlatformFactory.h"
#include "../CredentialService.h"
#include "WindowsSecretStore.h"
#include "WindowsGlobalShortcut.h"
#include "WindowsSelectionMonitor.h"
#include "WindowsWindowPolicy.h"
#include "WindowsSelectionResolver.h"
#include <QStringList>
namespace ws {
void preparePlatformEventLoop() {} // Qt configures per-monitor DPI before its first window.
std::unique_ptr<ISelectionMonitor> createSelectionMonitor(){return std::make_unique<WindowsSelectionMonitor>();}
std::unique_ptr<IPlatformWindowPolicy> createWindowPolicy(){return std::make_unique<WindowsWindowPolicy>();}
std::unique_ptr<IGlobalShortcut> createGlobalShortcut(){return std::make_unique<WindowsGlobalShortcut>();}
std::unique_ptr<IExplicitSelectionResolver> createExplicitResolver(ISelectionMonitor* monitor,QClipboard& clipboard){
    return std::make_unique<WindowsSelectionResolver>(monitor,clipboard);
}
std::unique_ptr<ISecretStore> createSecretStore(){
    QMap<QString,ISecretStore*> stores;
    for(const auto& id:QStringList{"openai","anthropic","gemini","openai_compatible"})
        stores.insert(id,new WindowsSecretStore(nullptr,id));
    return std::make_unique<CredentialService>(stores);
}
}
