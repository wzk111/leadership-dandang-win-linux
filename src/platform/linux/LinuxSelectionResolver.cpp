#include "LinuxSelectionResolver.h"
#include <QClipboard>
#include <QGuiApplication>
namespace ws {
LinuxSelectionResolver::LinuxSelectionResolver(ISelectionMonitor* monitor,QClipboard& clipboard)
    : monitor_(monitor), clipboard_(clipboard) {}
bool LinuxSelectionResolver::primarySupported() const {
    return QGuiApplication::platformName()=="xcb" && clipboard_.supportsSelection();
}
std::optional<ResolvedSelection> LinuxSelectionResolver::resolve() {
    SelectionResolver resolver(
        [this] {return monitor_ && monitor_->status().running?monitor_->latestSelection():std::optional<Selection>{};},
        [this]() -> std::optional<Selection> {
            if(!primarySupported()) return {};
            return Selection{clipboard_.text(QClipboard::Selection),{},"X11 PRIMARY"};
        },
        [this]() -> std::optional<Selection> {return Selection{clipboard_.text(QClipboard::Clipboard),{},"Clipboard"};});
    return resolver.resolve();
}
}
