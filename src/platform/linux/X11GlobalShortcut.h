#pragma once
#include "platform/IGlobalShortcut.h"
#include <memory>
namespace ws {
class X11ShortcutWorker;
class X11GlobalShortcut : public IGlobalShortcut {
    Q_OBJECT
public:
    X11GlobalShortcut();
    ~X11GlobalShortcut() override;
    void start(bool explicitEnable) override;
    void stop() override;
    GlobalShortcutStatus status() const override { return status_; }
private:
    std::unique_ptr<X11ShortcutWorker> worker_;
    GlobalShortcutStatus status_;
    quint64 generation_=0;
};
}
