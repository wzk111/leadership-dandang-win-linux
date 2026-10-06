#include "PortalGlobalShortcut.h"
namespace ws {
PortalGlobalShortcut::PortalGlobalShortcut(std::unique_ptr<PortalTransport> t):transport_(std::move(t)) {
    status_.backend="XDG Desktop Portal";
    connect(transport_.get(),&PortalTransport::probed,this,[this](uint version,const QString& error) {
        if(phase_!=Phase::Probe) return;
        status_.portalVersion=version; status_.available=version>=1;
        if(!status_.available) {fail(error.isEmpty()?"GlobalShortcuts portal unavailable; use tray or --trigger.":error);return;}
        if(!explicitEnable_) {
            phase_=Phase::Idle; status_.description="Available; explicitly re-enable shortcut to bind (no startup permission UI).";
            emit statusChanged(); return;
        }
        phase_=Phase::Create; status_.description="Creating portal session"; emit statusChanged(); transport_->create();
    });
    connect(transport_.get(),&PortalTransport::created,this,[this](const QString& session,const QString& error) {
        if(phase_!=Phase::Create) return;
        if(session.isEmpty() || !error.isEmpty()) {fail(error);return;}
        session_=session; phase_=Phase::Bind; status_.description="Waiting for portal shortcut configuration";
        emit statusChanged(); transport_->bind(session);
    });
    connect(transport_.get(),&PortalTransport::bound,this,[this](bool ok,const QString& trigger,const QString& error) {
        if(phase_!=Phase::Bind) return;
        if(!ok) {fail(error);return;}
        phase_=Phase::Registered; status_.registered=true;
        status_.triggerDescription=trigger.isEmpty()?"Registered (portal did not report trigger description)":trigger;
        status_.description="Registered"; emit statusChanged();
    });
    connect(transport_.get(),&PortalTransport::activation,this,[this](const QString& session,const QString& id) {
        if(phase_!=Phase::Registered || session!=session_ || id!=ShortcutId) return;
        status_.lastActivation=QDateTime::currentDateTimeUtc(); emit statusChanged(); emit activated();
    });
    connect(transport_.get(),&PortalTransport::lost,this,[this] {
        if(phase_==Phase::Idle && !status_.registered && !status_.available) return;
        status_.available=false; fail("Portal session lost; re-enable or use tray / --trigger.");
    });
}
PortalGlobalShortcut::~PortalGlobalShortcut() { stop(); }
void PortalGlobalShortcut::start(bool explicitEnable) {
    stop(); explicitEnable_=explicitEnable; phase_=Phase::Probe;
    status_.description="Probing GlobalShortcuts interface"; emit statusChanged(); transport_->probe();
}
void PortalGlobalShortcut::stop() {
    phase_=Phase::Idle; session_.clear(); transport_->close();
    status_.registered=false; status_.triggerDescription.clear(); status_.description="Disabled"; emit statusChanged();
}
void PortalGlobalShortcut::fail(const QString& message) {
    phase_=Phase::Idle; session_.clear(); transport_->close(); status_.registered=false;
    status_.triggerDescription.clear(); status_.description=message.isEmpty()?"Portal registration failed":message; emit statusChanged();
}
}
