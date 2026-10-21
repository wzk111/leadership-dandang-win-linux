#pragma once
#include "../IGlobalShortcut.h"
#include <QAbstractNativeEventFilter>
namespace ws {
class WindowsGlobalShortcut:public IGlobalShortcut,public QAbstractNativeEventFilter {
 Q_OBJECT
public:
    explicit WindowsGlobalShortcut(QObject* parent=nullptr);
    ~WindowsGlobalShortcut()override;
    void start(bool explicitEnable)override;
    void stop()override;
    GlobalShortcutStatus status()const override;
    bool nativeEventFilter(const QByteArray&,void*,qintptr*)override;
    int nativeId()const{return id_;}
    quintptr nativeWindow()const{return reinterpret_cast<quintptr>(window_);}
private:
    GlobalShortcutStatus state_;
    int id_=1;
    void* window_=nullptr;
    bool installed_=false;
};
}
