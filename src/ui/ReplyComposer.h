#pragma once
#include <QWidget>
#include "core/Core.h"
class QPlainTextEdit;
class QComboBox;
namespace ws {
class ReplyComposer : public QWidget {
 Q_OBJECT
public:
    ReplyComposer();
    void openSelection(const Selection&);
    std::optional<Selection> currentSelection() const {return selection_;}
signals:
    void generateRequested(const ws::Selection&,const QString& stance,const QString& intent);
protected:
    void hideEvent(QHideEvent*) override;
private:
    std::optional<Selection> selection_;
    QPlainTextEdit* preview_=nullptr;
    QPlainTextEdit* intent_=nullptr;
    QComboBox* stance_=nullptr;
};
}
