#pragma once
#include "platform/ISelectionMonitor.h"
#include <QTimer>
#include <memory>
namespace ws {
class UIAQuery;
struct WindowsMonitorCallbacks;
class WindowsSelectionMonitor : public ISelectionMonitor {
    Q_OBJECT
public:
    explicit WindowsSelectionMonitor(std::unique_ptr<UIAQuery> query,QObject* parent=nullptr);
    explicit WindowsSelectionMonitor(QObject* parent=nullptr);
    ~WindowsSelectionMonitor() override;
    bool start() override;
    void stop() override;
    MonitorStatus status()const override{return status_;}
    std::optional<Selection> latestSelection()const override{return latest_;}
    SelectionSource selectionSource()const override{return SelectionSource::UIA;}
    QString backendName()const override{return "Microsoft UI Automation (mouse gestures; explicit shortcut for keyboard selections)";}
protected:
    // Narrow native seams allow lifecycle tests without installing global hooks.
    virtual bool installHooks();
    virtual void releaseHooks();
    virtual quintptr foregroundWindow()const;
    virtual bool isExternalWindow(quintptr hwnd)const;
    void processMouse(bool down,QPoint point,quint32 timestamp,quintptr target);
    void processForeground(quintptr hwnd);
private:
    friend struct WindowsMonitorCallbacks;
    void invalidate(const QString& reason);
    void requestSelection();
    std::unique_ptr<UIAQuery> query_;
    QMetaObject::Connection completion_;
    QTimer debounce_,expiry_;
    MonitorStatus status_;
    std::optional<Selection> latest_;
    quint64 generation_=0;
    quint64 hookEpoch_=0;
    quintptr target_=0,lastForeground_=0,downTarget_=0,lastClickTarget_=0;
    QPoint downPoint_,lastClickPoint_;
    quint32 lastClickTime_=0;
    bool pressed_=false,doubleClick_=false,hasLastClick_=false;
    void* mouseHook_=nullptr;
    void* foregroundHook_=nullptr;
};
}
