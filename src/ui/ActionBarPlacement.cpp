#include "ActionBarPlacement.h"
#include <algorithm>
namespace ws {
std::optional<BarPlacement> placeActionBar(const QRect& anchor, const QSize& size, const QRect& available) {
    if (!anchor.isValid() || size.isEmpty() || !available.isValid() ||
        size.width() > available.width() || size.height() > available.height()) return {};
    // AT-SPI coordinates remain raw. No unverified DPI conversion.
    constexpr qint64 gap = 8;
    qint64 x = qint64(anchor.x()) + (qint64(anchor.width()) - size.width()) / 2;
    qint64 y = qint64(anchor.y()) - gap - size.height();
    QString mode = "ABOVE_SELECTION";
    if (y < available.y()) {
        y = qint64(anchor.y()) + anchor.height() + gap;
        mode = "BELOW_SELECTION";
    }
    const auto clampedX = std::clamp(x, qint64(available.x()),
        qint64(available.x()) + available.width() - size.width());
    const auto clampedY = std::clamp(y, qint64(available.y()),
        qint64(available.y()) + available.height() - size.height());
    if (x != clampedX || y != clampedY) mode += "_CLAMPED";
    return BarPlacement{QRect(QPoint(int(clampedX), int(clampedY)), size), mode};
}
}
