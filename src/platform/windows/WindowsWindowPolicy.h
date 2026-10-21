#pragma once
#include "platform/IPlatformWindowPolicy.h"
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QSet>
namespace ws {
class WindowsWindowPolicy : public QObject,public IPlatformWindowPolicy,public QAbstractNativeEventFilter {
public:
    explicit WindowsWindowPolicy(QObject* parent=nullptr);
    ~WindowsWindowPolicy() override;
    OverlayCapability overlayCapability()const override;
    bool configureActionBar(QWidget&)override;
    std::optional<BarPlacement> positionActionBar(QWidget&,const QRect&)override;
    QString description()const override;
    static std::optional<QRect> physicalToLogical(const QRect& anchor,const QRect& physicalMonitor,
        const QRect& logicalMonitor,double scale);
    bool nativeEventFilter(const QByteArray&,void*,qintptr*)override;
protected:
    bool eventFilter(QObject*,QEvent*)override;
private:
    bool applyStyles(QWidget&);
    QSet<QWidget*> windows_;
};
}
