#pragma once
#include "platform/IPlatformWindowPolicy.h"
namespace ws {
class LinuxWindowPolicy : public IPlatformWindowPolicy {
public:
    static OverlayCapability capabilityForBackend(const QString& backend);
    OverlayCapability overlayCapability() const override;
    bool configureActionBar(QWidget& window) override;
    std::optional<BarPlacement> positionActionBar(QWidget& window, const QRect& anchor) override;
    QString description() const override;
};
}
