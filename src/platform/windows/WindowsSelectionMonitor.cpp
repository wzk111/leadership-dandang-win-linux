#include "WindowsSelectionMonitor.h"
#include "UIAQuery.h"
#include <QThread>
#include <QCoreApplication>
#include <windows.h>
#include <cmath>
namespace ws {
namespace {
// Hooks are delivered on their installing Qt thread, which owns this pointer.
WindowsSelectionMonitor* installedMonitor=nullptr;
quintptr rootWindow(HWND hwnd) {return reinterpret_cast<quintptr>(GetAncestor(hwnd,GA_ROOT));}
}
struct WindowsMonitorCallbacks {
    static LRESULT CALLBACK mouse(int code,WPARAM message,LPARAM data) {
        if(code==HC_ACTION && installedMonitor && (message==WM_LBUTTONDOWN || message==WM_LBUTTONUP)) {
            const auto& event=*reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
            const QPoint point(event.pt.x,event.pt.y);const quint32 timestamp=event.time;
            const auto target=rootWindow(WindowFromPoint(event.pt));
            auto* monitor=installedMonitor;
            const auto epoch=monitor->hookEpoch_;
            // Copy only the gesture metadata; no UIA, text, or clipboard access here.
            QMetaObject::invokeMethod(monitor,[monitor,epoch,point,timestamp,target,down=message==WM_LBUTTONDOWN] {
                if(monitor->status_.running && monitor->hookEpoch_==epoch)
                    monitor->processMouse(down,point,timestamp,target);
            },Qt::QueuedConnection);
        }
        return CallNextHookEx(nullptr,code,message,data);
    }
    static void CALLBACK foreground(HWINEVENTHOOK,DWORD,HWND hwnd,LONG,LONG,DWORD,DWORD) {
        if(!installedMonitor)return;
        auto* monitor=installedMonitor;
        const auto epoch=monitor->hookEpoch_;
        QMetaObject::invokeMethod(monitor,[monitor,epoch,window=reinterpret_cast<quintptr>(hwnd)] {
            if(monitor->status_.running && monitor->hookEpoch_==epoch)monitor->processForeground(window);
        },Qt::QueuedConnection);
    }
};
WindowsSelectionMonitor::WindowsSelectionMonitor(QObject* parent)
    :WindowsSelectionMonitor(std::make_unique<UIAQuery>(),parent){}
WindowsSelectionMonitor::WindowsSelectionMonitor(std::unique_ptr<UIAQuery> query,QObject* parent)
    :ISelectionMonitor(parent),query_(std::move(query)),debounce_(this),expiry_(this) {
    debounce_.setSingleShot(true);debounce_.setInterval(80);
    expiry_.setSingleShot(true);expiry_.setInterval(45000);
    expiry_.setObjectName("selectionExpiry");
    connect(&debounce_,&QTimer::timeout,this,&WindowsSelectionMonitor::requestSelection);
    connect(&expiry_,&QTimer::timeout,this,[this]{invalidate("Selection expired");});
}
WindowsSelectionMonitor::~WindowsSelectionMonitor(){stop();}
bool WindowsSelectionMonitor::installHooks() {
    if(installedMonitor && installedMonitor!=this)return false;
    installedMonitor=this;
    mouseHook_=SetWindowsHookExW(WH_MOUSE_LL,&WindowsMonitorCallbacks::mouse,GetModuleHandleW(nullptr),0);
    // Own-process foreground changes must also invalidate old external context.
    foregroundHook_=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,
        &WindowsMonitorCallbacks::foreground,0,0,WINEVENT_OUTOFCONTEXT);
    if(!mouseHook_ || !foregroundHook_){releaseHooks();return false;}
    return true;
}
void WindowsSelectionMonitor::releaseHooks() {
    if(mouseHook_){UnhookWindowsHookEx(static_cast<HHOOK>(mouseHook_));mouseHook_=nullptr;}
    if(foregroundHook_){UnhookWinEvent(static_cast<HWINEVENTHOOK>(foregroundHook_));foregroundHook_=nullptr;}
    if(installedMonitor==this)installedMonitor=nullptr;
}
quintptr WindowsSelectionMonitor::foregroundWindow()const{return reinterpret_cast<quintptr>(GetForegroundWindow());}
bool WindowsSelectionMonitor::isExternalWindow(quintptr window)const {
    const auto hwnd=reinterpret_cast<HWND>(window);DWORD process=0;
    return hwnd && IsWindow(hwnd) && GetWindowThreadProcessId(hwnd,&process) && process && process!=GetCurrentProcessId();
}
bool WindowsSelectionMonitor::start() {
    if(status_.running)return true;
    if(QThread::currentThread()!=thread() || !query_ || !installHooks()) {
        status_.available=false;status_.description="Windows selection hooks unavailable; use explicit clipboard fallback";
        emit statusChanged();return false;
    }
    ++generation_;++hookEpoch_;status_.running=true;status_.available=true;status_.initialized=false;
    status_.description="Mouse gesture monitor running; UIA initialized on next selection; keyboard selections use explicit shortcut";
    lastForeground_=foregroundWindow();emit statusChanged();return true;
}
void WindowsSelectionMonitor::stop() {
    status_.running=false;++hookEpoch_;releaseHooks();pressed_=false;hasLastClick_=false;downTarget_=0;
    invalidate("Stopped");status_.description="Stopped";status_.initialized=false;emit statusChanged();
}
void WindowsSelectionMonitor::invalidate(const QString& reason) {
    ++generation_;debounce_.stop();expiry_.stop();disconnect(completion_);completion_={};
    if(query_)query_->cancel();target_=0;
    latest_.reset();status_.lastResult=reason;emit selectionCleared();emit statusChanged();
}
void WindowsSelectionMonitor::processForeground(quintptr window) {
    if(!status_.running || window==lastForeground_)return;
    lastForeground_=window;if(downTarget_!=window)pressed_=false;hasLastClick_=false;
    invalidate("Foreground window changed");
}
void WindowsSelectionMonitor::processMouse(bool down,QPoint point,quint32 timestamp,quintptr window) {
    if(!status_.running)return;
    // In particular, clicking our ActionBar must not clear its captured source.
    if(!isExternalWindow(window)){pressed_=false;return;}
    if(down) {
        const bool second=hasLastClick_ && window==lastClickTarget_ &&
            quint32(timestamp-lastClickTime_)<=GetDoubleClickTime() &&
            std::abs(point.x()-lastClickPoint_.x())<=GetSystemMetrics(SM_CXDOUBLECLK)/2 &&
            std::abs(point.y()-lastClickPoint_.y())<=GetSystemMetrics(SM_CYDOUBLECLK)/2;
        invalidate("External pointer gesture");
        downPoint_=point;downTarget_=window;pressed_=true;doubleClick_=second;
        return;
    }
    if(!pressed_ || downTarget_!=window)return;
    pressed_=false;
    const bool dragged=std::abs(point.x()-downPoint_.x())>=GetSystemMetrics(SM_CXDRAG) ||
        std::abs(point.y()-downPoint_.y())>=GetSystemMetrics(SM_CYDRAG);
    hasLastClick_=!dragged;lastClickPoint_=point;lastClickTime_=timestamp;lastClickTarget_=window;
    if(!dragged && !doubleClick_)return;
    target_=window;++status_.events;status_.lastEvent=QDateTime::currentDateTimeUtc();
    debounce_.start();emit statusChanged();
}
void WindowsSelectionMonitor::requestSelection() {
    if(!status_.running || !target_ || !isExternalWindow(target_) || foregroundWindow()!=target_) {
        invalidate("No valid external foreground selection");return;
    }
    const auto generation=generation_;const auto target=target_;
    disconnect(completion_);
    completion_=connect(query_.get(),&UIAQuery::completed,this,[this,generation,target](const UIAQueryResult& result) {
        if(!status_.running || generation!=generation_ || target_!=target || foregroundWindow()!=target)return;
        disconnect(completion_);completion_={};status_.initialized=result.initialized;
        const bool valid=result.initialized && result.selection && !result.selection->text.trimmed().isEmpty() && result.selection->text.size()<=100000;
        if(!valid){
            // UIAQuery emits fixed diagnostics or whitelist-validated worker reasons,
            // never source text or raw helper output.
            invalidate(result.reason.isEmpty()?(result.initialized?"No supported nonempty selection":"UI Automation initialization unavailable"):result.reason);
            return;
        }
        latest_=result.selection;status_.lastResult="Selected text available locally";
        expiry_.start();const auto snapshot=*latest_;emit selectionDetected(snapshot);emit statusChanged();
    });
    if(!query_->request(target)){invalidate("UI Automation query unavailable or busy");}
}
}
