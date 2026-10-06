#pragma once
#include <QWidget>
#include "ui/ActionBarPlacement.h"
namespace ws {
enum class OverlayCapability { AnchoredNonActivating, Unsupported };
class IPlatformWindowPolicy {
public:
    virtual ~IPlatformWindowPolicy() = default;
    virtual OverlayCapability overlayCapability() const = 0;
    virtual bool configureActionBar(QWidget& window) = 0;
    virtual std::optional<BarPlacement> positionActionBar(QWidget& window, const QRect& anchor) = 0;
    virtual QString description() const = 0;
};
}
