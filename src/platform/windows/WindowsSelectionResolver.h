#pragma once
#include "core/SelectionResolver.h"
#include "platform/ISelectionMonitor.h"
#include <QPointer>
#include <memory>
class QClipboard;
namespace ws {
class UIAQuery;
class WindowsSelectionResolver:public QObject,public IExplicitSelectionResolver {
 Q_OBJECT
public:
    WindowsSelectionResolver(ISelectionMonitor*,QClipboard&,QObject* parent=nullptr);
    WindowsSelectionResolver(ISelectionMonitor*,QClipboard&,std::unique_ptr<UIAQuery> query,QObject* parent=nullptr);
    ~WindowsSelectionResolver()override;
    std::optional<ResolvedSelection> resolve()override;
    void resolveAsync(QObject*,Completion)override;
    void cancel()override;
    QString description()const override{return "UI Automation → Clipboard";}
protected:
    virtual quintptr foregroundWindow()const;
private:
    std::optional<ResolvedSelection> choose(std::optional<Selection>);
    QPointer<ISelectionMonitor> monitor_;
    QClipboard& clipboard_;
    std::unique_ptr<UIAQuery> query_;
    quint64 generation_=0;
    QMetaObject::Connection completion_,destroyed_;
};
}
