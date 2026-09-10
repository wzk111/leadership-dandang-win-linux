#include "AtSpiSelectionMonitor.h"
namespace ws {
AtSpiSelectionMonitor::AtSpiSelectionMonitor(std::unique_ptr<IAtSpiSession> session, QObject* parent)
    : ISelectionMonitor(parent), session_(std::move(session)) {
    debounce_.setSingleShot(true); debounce_.setInterval(80);
    expiry_.setSingleShot(true); expiry_.setInterval(45000);
    connect(&debounce_, &QTimer::timeout, this, [this] { if (enabled_ && status_.running) session_->extract(sequence_); });
    connect(&expiry_, &QTimer::timeout, this, [this] { clear(); status_.lastResult="Selection expired after 45 seconds"; emit statusChanged(); });
    connect(session_.get(), &IAtSpiSession::state, this, [this](const MonitorStatus& s) {
        if (!enabled_) return;
        status_.available=s.available; status_.initialized=s.initialized;
        status_.running=s.running; status_.description=s.description;
        if (!s.running) { debounce_.stop(); clear(); }
        emit statusChanged();
    });
    connect(session_.get(), &IAtSpiSession::eventReceived, this, [this](quint64 sequence) {
        if (!enabled_ || !status_.running) return;
        sequence_=sequence; ++status_.events; status_.lastEvent=QDateTime::currentDateTimeUtc();
        debounce_.start();
    });
    connect(session_.get(), &IAtSpiSession::extracted, this, [this](quint64 sequence, const SelectionRead& r) {
        if (!enabled_ || !status_.running || sequence != sequence_) return;
        if (r.ignored) return;
        status_.lastResult = r.description;
        if (!r.selection) clear();
        else {
            const bool same = latest_ && latest_->text == r.selection->text &&
                latest_->anchorRect == r.selection->anchorRect && latest_->sourceApplication == r.selection->sourceApplication;
            latest_=r.selection; expiry_.start();
            if (!same) emit selectionDetected(*latest_);
        }
        emit statusChanged();
    });
}
AtSpiSelectionMonitor::~AtSpiSelectionMonitor() { stop(); }
bool AtSpiSelectionMonitor::start() {
    if (enabled_) return true;
    enabled_=true; sequence_=0; status_.description="Starting";
    emit statusChanged(); session_->start(); return true;
}
void AtSpiSelectionMonitor::stop() {
    if (!enabled_) return;
    enabled_=false; debounce_.stop(); clear(); session_->stop();
    status_.running=false; status_.description="Stopped"; emit statusChanged();
}
void AtSpiSelectionMonitor::clear() {
    expiry_.stop(); latest_.reset(); emit selectionCleared();
}
}
