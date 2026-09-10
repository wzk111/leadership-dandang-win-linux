#pragma once
#include <QWidget>
#include "platform/ISelectionMonitor.h"
class QPlainTextEdit;
namespace ws {
class SelectionDiagnostics : public QWidget {
    Q_OBJECT
public:
    explicit SelectionDiagnostics(ISelectionMonitor* monitor, QWidget* parent=nullptr);
    void refresh();
protected:
    void hideEvent(QHideEvent* event) override;
private:
    ISelectionMonitor* monitor_;
    QPlainTextEdit* metadata_;
    QPlainTextEdit* preview_;
};
}
