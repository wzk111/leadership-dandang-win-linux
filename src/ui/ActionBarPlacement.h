#pragma once
#include <QRect>
#include <QString>
#include <optional>
namespace ws {
struct BarPlacement { QRect geometry; QString mode; };
std::optional<BarPlacement> placeActionBar(const QRect& anchor, const QSize& size, const QRect& available);
}
