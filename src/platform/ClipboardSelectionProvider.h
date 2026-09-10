#pragma once
#include "ISelectionProvider.h"
#include <QClipboard>
namespace ws {
// Called only by Process Clipboard. No monitoring, simulated Ctrl+C or history.
class ClipboardSelectionProvider : public ISelectionProvider {
public:
    explicit ClipboardSelectionProvider(QClipboard& clipboard) : clipboard_(clipboard) {}
    std::optional<Selection> currentSelection() override;
private:
    QClipboard& clipboard_;
};
}
