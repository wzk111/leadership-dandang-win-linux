#pragma once
#include <QWidget>
#include "core/Core.h"
#include "core/FeaturePreferences.h"
class QPlainTextEdit;
class QLabel;
class QPushButton;
namespace ws {
class WorkspaceWindow : public QWidget {
    Q_OBJECT
public:
    explicit WorkspaceWindow(QWidget* parent = nullptr);
    void setCaptured(const QString& text);
    void setBusy(bool busy);
    void setPreferences(const FeaturePreferences& preferences);
    void setTrayAvailable(bool available);
signals:
    void processClipboard();
    void featureChosen(ws::Feature feature);
    void settingsRequested();
    void diagnosticsRequested();
private:
    QPlainTextEdit* preview_;
    QLabel* count_;
    QLabel* trayNotice_;
    QPushButton* process_;
    QList<QPushButton*> features_;
    bool busy_ = false;
};
}
