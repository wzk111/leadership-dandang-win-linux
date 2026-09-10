#pragma once
#include "../ISelectionMonitor.h"
#include "IAtSpiSession.h"
#include <QTimer>
#include <memory>
namespace ws {
class AtSpiSelectionMonitor : public ISelectionMonitor {
    Q_OBJECT
public:
    explicit AtSpiSelectionMonitor(std::unique_ptr<IAtSpiSession> session, QObject* parent = nullptr);
    ~AtSpiSelectionMonitor() override;
    bool start() override;
    void stop() override;
    MonitorStatus status() const override { return status_; }
    std::optional<Selection> latestSelection() const override { return latest_; }
    QString backendName() const override { return "AT-SPI2"; }
private:
    std::unique_ptr<IAtSpiSession> session_;
    MonitorStatus status_;
    QTimer debounce_;
    QTimer expiry_;
    std::optional<Selection> latest_;
    quint64 sequence_ = 0;
    bool enabled_ = false;
    void clear();
};
std::unique_ptr<IAtSpiSession> createAtSpiSession();
}
