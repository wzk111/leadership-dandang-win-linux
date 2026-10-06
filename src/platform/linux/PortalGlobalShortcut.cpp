#include "PortalGlobalShortcut.h"
namespace ws {
PortalGlobalShortcut::PortalGlobalShortcut(std::unique_ptr<PortalTransport> t):transport_(std::move(t)) {status_.backend="XDG Desktop Portal";}
PortalGlobalShortcut::~PortalGlobalShortcut() = default;
void PortalGlobalShortcut::start(bool) {}
void PortalGlobalShortcut::stop() {}
void PortalGlobalShortcut::fail(const QString&) {}
}
