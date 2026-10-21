#include "WindowsGlobalShortcut.h"
#include <QCoreApplication>
#include <windows.h>
namespace ws {
WindowsGlobalShortcut::WindowsGlobalShortcut(QObject* parent):IGlobalShortcut(parent) {
    state_.backend="Win32 RegisterHotKey";state_.available=true;state_.triggerDescription=PreferredShortcut;
}
WindowsGlobalShortcut::~WindowsGlobalShortcut(){stop();}
void WindowsGlobalShortcut::start(bool) {
    if(state_.registered)return;
    if(!id_ || !QCoreApplication::instance()) {state_.description="Shortcut registration unavailable.";emit statusChanged();return;}
    // A private message-only HWND scopes an application-range ID without colliding
    // with another component's thread-level registration or DLL atom IDs.
    window_=CreateWindowExW(0,L"STATIC",L"WorkSidekick hotkey",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window_) {state_.description="Shortcut message window unavailable.";emit statusChanged();return;}
    if(!RegisterHotKey(static_cast<HWND>(window_),id_,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'P')) {
        DestroyWindow(static_cast<HWND>(window_));window_=nullptr;
        state_.description="Ctrl+Alt+P is unavailable or already registered. Use the tray or clipboard workflow.";
        emit statusChanged();return;
    }
    QCoreApplication::instance()->installNativeEventFilter(this);installed_=true;
    state_.registered=true;state_.description="Ctrl+Alt+P registered; explicit actions only.";emit statusChanged();
}
void WindowsGlobalShortcut::stop() {
    if(installed_ && QCoreApplication::instance())QCoreApplication::instance()->removeNativeEventFilter(this);
    installed_=false;
    if(state_.registered)UnregisterHotKey(static_cast<HWND>(window_),id_);
    if(window_){DestroyWindow(static_cast<HWND>(window_));window_=nullptr;}
    state_.registered=false;state_.description="Disabled";emit statusChanged();
}
GlobalShortcutStatus WindowsGlobalShortcut::status()const{return state_;}
bool WindowsGlobalShortcut::nativeEventFilter(const QByteArray&,void* native,qintptr* result) {
    if(!state_.registered || !native)return false;
    const auto* message=static_cast<MSG*>(native);
    if(message->message!=WM_HOTKEY || message->wParam!=WPARAM(id_) || message->hwnd!=static_cast<HWND>(window_))return false;
    if(HIWORD(message->lParam)!='P' || (LOWORD(message->lParam)&(MOD_CONTROL|MOD_ALT))!=(MOD_CONTROL|MOD_ALT))return false;
    state_.lastActivation=QDateTime::currentDateTimeUtc();emit statusChanged();emit activated();
    if(result)*result=0;return true;
}
}
