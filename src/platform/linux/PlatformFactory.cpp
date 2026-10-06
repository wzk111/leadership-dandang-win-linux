#include "../PlatformFactory.h"
#include "LinuxSecretStore.h"
#include "AtSpiSelectionMonitor.h"
#include "LinuxWindowPolicy.h"
#include "LinuxSelectionResolver.h"
#include "X11GlobalShortcut.h"
#include "PortalGlobalShortcut.h"
#include "QtPortalTransport.h"
#include <QGuiApplication>
#include <X11/Xlib.h>
namespace ws {
std::unique_ptr<IPlatformWindowPolicy> createWindowPolicy() { return std::make_unique<LinuxWindowPolicy>(); }
void preparePlatformEventLoop() { XInitThreads(); qputenv("QT_NO_GLIB", "1"); }
std::unique_ptr<ISelectionMonitor> createSelectionMonitor() {
    return std::make_unique<AtSpiSelectionMonitor>(createAtSpiSession());
}
class UnavailableShortcut : public IGlobalShortcut {
public:
    void start(bool) override { emit statusChanged(); }
    void stop() override {}
    GlobalShortcutStatus status() const override {
        GlobalShortcutStatus s; s.description="Global shortcut unavailable on current Qt backend; use tray or --trigger."; return s;
    }
};
std::unique_ptr<IGlobalShortcut> createGlobalShortcut() {
    const auto backend=QGuiApplication::platformName();
    if(backend=="xcb") return std::make_unique<X11GlobalShortcut>();
    if(backend.startsWith("wayland")) return std::make_unique<PortalGlobalShortcut>(std::make_unique<QtPortalTransport>());
    return std::make_unique<UnavailableShortcut>();
}
std::unique_ptr<IExplicitSelectionResolver> createExplicitResolver(ISelectionMonitor* monitor,QClipboard& clipboard) {
    return std::make_unique<LinuxSelectionResolver>(monitor,clipboard);
}
std::unique_ptr<ISecretStore> createSecretStore() { return std::make_unique<LinuxSecretStore>(); }
}
