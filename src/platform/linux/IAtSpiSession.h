#pragma once
#include "../ISelectionMonitor.h"
#include "AtSpiSelectionProvider.h"
namespace ws {
// Transport boundary to the native worker. All methods and signals on the GUI
// side carry Qt values only; native accessible refs stay on the worker thread.
class IAtSpiSession : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void extract(quint64 sequence) = 0;
signals:
    void state(const ws::MonitorStatus& status);
    void eventReceived(quint64 sequence);
    void extracted(quint64 sequence, const ws::SelectionRead& result);
};
}
