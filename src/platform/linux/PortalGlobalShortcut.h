#pragma once
#include "platform/IGlobalShortcut.h"
#include "PortalTransport.h"
#include <memory>
namespace ws {
class PortalGlobalShortcut : public IGlobalShortcut {
    Q_OBJECT
public:
    explicit PortalGlobalShortcut(std::unique_ptr<PortalTransport> transport);
    ~PortalGlobalShortcut() override;
    void start(bool explicitEnable) override;
    void stop() override;
    GlobalShortcutStatus status() const override { return status_; }
private:
    enum class Phase { Idle, Probe, Create, Bind, Registered };
    Phase phase_=Phase::Idle;
    bool explicitEnable_=false;
    QString session_;
    std::unique_ptr<PortalTransport> transport_;
    GlobalShortcutStatus status_;
    void fail(const QString& message);
};
}
