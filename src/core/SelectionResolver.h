#pragma once
#include "Core.h"
#include <functional>
namespace ws {
enum class SelectionSource { AtSpi, PrimarySelection, Clipboard };
struct ResolvedSelection { Selection selection; SelectionSource source; };
QString selectionSourceName(SelectionSource source);
class IExplicitSelectionResolver {
public:
    virtual ~IExplicitSelectionResolver() = default;
    virtual std::optional<ResolvedSelection> resolve() = 0;
    virtual bool primarySupported() const { return false; }
};
class SelectionResolver : public IExplicitSelectionResolver {
public:
    using Read = std::function<std::optional<Selection>()>;
    SelectionResolver(Read atspi, Read primary, Read clipboard);
    std::optional<ResolvedSelection> resolve() override;
private:
    Read atspi_, primary_, clipboard_;
};
}
