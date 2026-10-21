#pragma once
#include "platform/ISelectionProvider.h"
namespace ws {
// Synchronous COM adapter: invoke only inside the windowless MTA worker process.
// Geometry is physical screen pixels; the Windows window policy converts it once.
class UIASelectionProvider final:public ISelectionProvider {
public:
    UIASelectionProvider(quintptr targetWindow,quint32 excludedProcessId);
    std::optional<Selection> currentSelection() override;
    bool initialized()const{return initialized_;}
    QString reason()const{return reason_;}
    static std::optional<QString> joinRanges(const QStringList& ranges);
    static std::optional<QRect> lastRectangle(const QList<double>& coordinates);
private:
    quintptr targetWindow_;
    quint32 excludedProcessId_;
    bool initialized_=false;
    QString reason_="not queried";
};
}
