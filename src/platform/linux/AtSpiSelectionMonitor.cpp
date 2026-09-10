#include "AtSpiSelectionMonitor.h"
namespace ws {
AtSpiSelectionMonitor::AtSpiSelectionMonitor(std::unique_ptr<IAtSpiSession> session, QObject* parent)
    : ISelectionMonitor(parent), session_(std::move(session)) {}
AtSpiSelectionMonitor::~AtSpiSelectionMonitor() = default;
bool AtSpiSelectionMonitor::start() { return false; }
void AtSpiSelectionMonitor::stop() {}
void AtSpiSelectionMonitor::clear() {}
}
