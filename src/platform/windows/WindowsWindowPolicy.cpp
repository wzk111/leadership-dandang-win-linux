#include "WindowsWindowPolicy.h"
#include <QGuiApplication>
#include <QScreen>
#include <QWindowsScreen>
#include <QEvent>
#include <windows.h>
#include <cmath>
#include <climits>
#include <limits>
namespace ws {
WindowsWindowPolicy::WindowsWindowPolicy(QObject* parent):QObject(parent) {
    if(QCoreApplication::instance())QCoreApplication::instance()->installNativeEventFilter(this);
}
WindowsWindowPolicy::~WindowsWindowPolicy() {
    if(QCoreApplication::instance())QCoreApplication::instance()->removeNativeEventFilter(this);
}
OverlayCapability WindowsWindowPolicy::overlayCapability()const {
    return QGuiApplication::platformName()=="windows"?OverlayCapability::AnchoredNonActivating:OverlayCapability::Unsupported;
}
bool WindowsWindowPolicy::applyStyles(QWidget& window) {
    const auto hwnd=reinterpret_cast<HWND>(window.winId());
    const auto oldStyle=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    SetLastError(0);
    const auto previous=SetWindowLongPtrW(hwnd,GWL_EXSTYLE,oldStyle|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);
    if(!previous && GetLastError()!=0)return false;
    return SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED)!=FALSE;
}
bool WindowsWindowPolicy::configureActionBar(QWidget& window) {
    if(overlayCapability()!=OverlayCapability::AnchoredNonActivating)return false;
    window.setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowDoesNotAcceptFocus);
    window.setAttribute(Qt::WA_ShowWithoutActivating);window.setFocusPolicy(Qt::NoFocus);
    if(!windows_.contains(&window)) {
        windows_.insert(&window);window.installEventFilter(this);
        connect(&window,&QObject::destroyed,this,[this,p=&window]{windows_.remove(p);});
    }
    return applyStyles(window);
}
bool WindowsWindowPolicy::eventFilter(QObject* object,QEvent* event) {
    if(event->type()==QEvent::Show || event->type()==QEvent::WinIdChange) {
        auto* window=qobject_cast<QWidget*>(object);
        if(window && window->internalWinId() && windows_.contains(window))applyStyles(*window);
    }
    return QObject::eventFilter(object,event);
}
bool WindowsWindowPolicy::nativeEventFilter(const QByteArray& type,void* message,qintptr* result) {
    if(type!="windows_generic_MSG" && type!="windows_dispatcher_MSG")return false;
    const auto* native=static_cast<MSG*>(message);
    if(!native || native->message!=WM_MOUSEACTIVATE)return false;
    for(auto* window:windows_)if(reinterpret_cast<HWND>(window->internalWinId())==native->hwnd) {
        // MA_NOACTIVATE keeps the click; MA_NOACTIVATEANDEAT would discard it.
        *result=MA_NOACTIVATE;return true;
    }
    return false;
}
std::optional<QRect> WindowsWindowPolicy::physicalToLogical(const QRect& anchor,const QRect& physical,
    const QRect& logical,double scale) {
    if(!anchor.isValid() || !physical.isValid() || !logical.isValid() || !std::isfinite(scale) || scale<=0)return {};
    // Screen origins are already Qt virtual-desktop positions; scale local offsets only.
    const double left=std::floor(logical.x()+(double(anchor.x())-physical.x())/scale);
    const double top=std::floor(logical.y()+(double(anchor.y())-physical.y())/scale);
    const double right=std::ceil(logical.x()+(double(anchor.x())+anchor.width()-physical.x())/scale);
    const double bottom=std::ceil(logical.y()+(double(anchor.y())+anchor.height()-physical.y())/scale);
    const auto low=std::numeric_limits<int>::min(),high=std::numeric_limits<int>::max();
    if(left<low || top<low || right>high || bottom>high || right-left>high || bottom-top>high)return {};
    return QRect(int(left),int(top),int(right-left),int(bottom-top));
}
std::optional<BarPlacement> WindowsWindowPolicy::positionActionBar(QWidget& window,const QRect& anchor) {
    if(overlayCapability()!=OverlayCapability::AnchoredNonActivating || !anchor.isValid())return {};
    const auto right=qint64(anchor.x())+anchor.width(),bottom=qint64(anchor.y())+anchor.height();
    if(right>LONG_MAX || bottom>LONG_MAX)return {};
    RECT rectangle{anchor.x(),anchor.y(),LONG(right),LONG(bottom)};
    const auto monitor=MonitorFromRect(&rectangle,MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW info{};info.cbSize=sizeof(info);
    if(!monitor || !GetMonitorInfoW(monitor,reinterpret_cast<MONITORINFO*>(&info)))return {};
    QScreen* screen=nullptr;
    for(auto* candidate:QGuiApplication::screens()) {
        const auto* native=candidate->nativeInterface<QNativeInterface::QWindowsScreen>();
        if(native && native->handle()==monitor){screen=candidate;break;}
    }
    // Do not guess the primary screen for an unmatched mixed-DPI monitor.
    if(!screen)return {};
    const QRect physical(info.rcMonitor.left,info.rcMonitor.top,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top);
    const auto logical=physicalToLogical(anchor,physical,screen->geometry(),screen->devicePixelRatio());
    if(!logical)return {};
    const auto placement=placeActionBar(*logical,window.size(),screen->availableGeometry());
    if(placement){window.move(placement->geometry.topLeft());if(!applyStyles(window))return {};}
    return placement;
}
QString WindowsWindowPolicy::description()const {
    return overlayCapability()==OverlayCapability::AnchoredNonActivating
        ? "WindowsNonActivating (UIA physical to Qt logical screen-local coordinates)"
        : "Unsupported: native Qt Windows window backend required";
}
}
