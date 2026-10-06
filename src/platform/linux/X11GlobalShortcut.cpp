#include "X11GlobalShortcut.h"
namespace ws {
class X11ShortcutWorker {};
X11GlobalShortcut::X11GlobalShortcut() { status_.backend="X11"; }
X11GlobalShortcut::~X11GlobalShortcut() = default;
void X11GlobalShortcut::start(bool) {}
void X11GlobalShortcut::stop() {}
}
