#pragma once
#include "Core.h"
#include <functional>
#include <QObject>
namespace ws {
enum class SelectionSource { AtSpi, PrimarySelection, Clipboard, UIA };
struct ResolvedSelection { Selection selection; SelectionSource source; };
QString selectionSourceName(SelectionSource source);
class IExplicitSelectionResolver {
public:
    virtual ~IExplicitSelectionResolver() = default;
    virtual std::optional<ResolvedSelection> resolve() = 0;
    using Completion = std::function<void(std::optional<ResolvedSelection>)>;
    virtual void resolveAsync(QObject*, Completion completed) { completed(resolve()); }
    virtual void cancel() {}
    virtual QString description() const { return "AT-SPI → X11 PRIMARY → Clipboard"; }
    virtual bool primarySupported() const { return false; }
};
class SelectionResolver : public IExplicitSelectionResolver {
public:
    using Read = std::function<std::optional<Selection>()>;
    SelectionResolver(Read atspi, Read primary, Read clipboard, SelectionSource preferredSource = SelectionSource::AtSpi);
    std::optional<ResolvedSelection> resolve() override;
private:
    Read atspi_, primary_, clipboard_;
    SelectionSource preferredSource_;
};
}
