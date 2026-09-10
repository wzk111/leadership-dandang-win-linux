#pragma once
#include <QObject>
#include <QDateTime>
#include "core/Core.h"
namespace ws {
struct MonitorStatus {
    bool available = false;
    bool initialized = false;
    bool running = false;
    QString description = "Stopped";
    quint64 events = 0;
    QDateTime lastEvent;
    QString lastResult = "No selection received";
};
class ISelectionMonitor : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual MonitorStatus status() const = 0;
    virtual std::optional<Selection> latestSelection() const = 0;
    virtual QString backendName() const = 0;
signals:
    void selectionDetected(const ws::Selection& selection);
    void selectionCleared();
    void statusChanged();
};
}
Q_DECLARE_METATYPE(ws::Selection)
Q_DECLARE_METATYPE(ws::MonitorStatus)
