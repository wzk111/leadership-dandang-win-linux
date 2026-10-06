#include "LinuxSelectionResolver.h"
#include <QClipboard>
#include <QGuiApplication>
namespace ws {
LinuxSelectionResolver::LinuxSelectionResolver(ISelectionMonitor* monitor,QClipboard& clipboard)
    : monitor_(monitor), clipboard_(clipboard) {}
bool LinuxSelectionResolver::primarySupported() const { return false; }
std::optional<ResolvedSelection> LinuxSelectionResolver::resolve() { return {}; }
}
