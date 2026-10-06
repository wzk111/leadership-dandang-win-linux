#pragma once
#include "core/SelectionResolver.h"
#include "platform/ISelectionMonitor.h"
class QClipboard;
namespace ws {
class LinuxSelectionResolver : public IExplicitSelectionResolver {
public:
    LinuxSelectionResolver(ISelectionMonitor* monitor, QClipboard& clipboard);
    std::optional<ResolvedSelection> resolve() override;
    bool primarySupported() const override;
private:
    ISelectionMonitor* monitor_;
    QClipboard& clipboard_;
};
}
