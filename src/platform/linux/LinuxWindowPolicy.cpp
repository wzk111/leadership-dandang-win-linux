#include "LinuxWindowPolicy.h"
#include <QGuiApplication>
#include <QScreen>
namespace ws {
OverlayCapability LinuxWindowPolicy::capabilityForBackend(const QString& backend) {
    return backend == "xcb" ? OverlayCapability::AnchoredNonActivating : OverlayCapability::Unsupported;
}
OverlayCapability LinuxWindowPolicy::overlayCapability() const {
    return capabilityForBackend(QGuiApplication::platformName());
}
bool LinuxWindowPolicy::configureActionBar(QWidget& window) {
    if (overlayCapability() != OverlayCapability::AnchoredNonActivating) return false;
    window.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    window.setAttribute(Qt::WA_ShowWithoutActivating);
    window.setFocusPolicy(Qt::NoFocus);
    return true;
}
std::optional<BarPlacement> LinuxWindowPolicy::positionActionBar(QWidget& window, const QRect& anchor) {
    if (overlayCapability() != OverlayCapability::AnchoredNonActivating || !anchor.isValid()) return {};
    auto* screen = QGuiApplication::screenAt(anchor.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return {};
    const auto placement = placeActionBar(anchor, window.size(), screen->availableGeometry());
    if (placement) window.move(placement->geometry.topLeft());
    return placement;
}
QString LinuxWindowPolicy::description() const {
    const auto backend = QGuiApplication::platformName();
    if (backend == "xcb") return "AnchoredNonActivating (Qt xcb; X11 / XWayland)";
    if (backend.startsWith("wayland"))
        return "Unsupported: native Wayland anchored positioning; automatic toolbar inactive; use manual clipboard mode";
    return "Unsupported: no verified anchored window policy for Qt backend " + backend;
}
}
