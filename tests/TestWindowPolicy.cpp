#include <QtTest>
#include "platform/linux/LinuxWindowPolicy.h"
using namespace ws;
class TestWindowPolicy : public QObject {
    Q_OBJECT
private slots:
    void backendNotSession() {
        qputenv("XDG_SESSION_TYPE","wayland");
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend("xcb"),OverlayCapability::AnchoredNonActivating);
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend("wayland"),OverlayCapability::Unsupported);
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend("wayland-egl"),OverlayCapability::Unsupported);
        qputenv("XDG_SESSION_TYPE","x11");
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend("wayland"),OverlayCapability::Unsupported);
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend("offscreen"),OverlayCapability::Unsupported);
        QCOMPARE(LinuxWindowPolicy::capabilityForBackend(""),OverlayCapability::Unsupported);
    }
};
QTEST_GUILESS_MAIN(TestWindowPolicy)
#include "TestWindowPolicy.moc"
