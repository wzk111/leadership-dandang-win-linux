#pragma once
#include "core/Core.h"
namespace ws {
class ISelectionProvider {
public:
    virtual ~ISelectionProvider() = default;
    virtual std::optional<Selection> currentSelection() = 0;
};
}
