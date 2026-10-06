#pragma once
#include <QWidget>
#include "core/SelectionResolver.h"
class QLabel;
class QPlainTextEdit;
class QPushButton;
namespace ws {
class ManualActionPalette : public QWidget {
    Q_OBJECT
public:
    ManualActionPalette();
    void showSelection(const ResolvedSelection& selection);
    void showMessage(const QString& message);
    std::optional<ResolvedSelection> currentSelection() const { return snapshot_; }
signals:
    void featureChosen(ws::Feature feature, const ws::Selection& selection);
protected:
    void hideEvent(QHideEvent* event) override;
private:
    void clear();
    std::optional<ResolvedSelection> snapshot_;
    QLabel* source_;
    QPlainTextEdit* preview_;
    QList<QPushButton*> buttons_;
};
}
