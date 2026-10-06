#pragma once
#include <QWidget>
#include "platform/ISelectionMonitor.h"
namespace ws {
class ActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ActionBar(QWidget* parent = nullptr);
    void showForSelection(const Selection& selection, const QPoint& position);
    void hideBar();
    std::optional<Selection> currentSelection() const { return selection_; }
signals:
    void featureChosen(ws::Feature feature, const ws::Selection& selection);
    void dismissed();
protected:
    void hideEvent(QHideEvent* event) override;
private:
    std::optional<Selection> selection_;
};
}
