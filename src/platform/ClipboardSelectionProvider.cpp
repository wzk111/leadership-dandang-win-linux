#include "ClipboardSelectionProvider.h"
namespace ws {
std::optional<Selection> ClipboardSelectionProvider::currentSelection() {
    const auto text = clipboard_.text(QClipboard::Clipboard);
    if (text.trimmed().isEmpty()) return std::nullopt;
    return Selection{text, {}, "Manual clipboard"};
}
}
