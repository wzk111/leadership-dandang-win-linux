#pragma once
#include <QObject>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QWidget>
#include "AppController.h"
#include "ui/WorkspaceWindow.h"
#include "ui/SettingsWindow.h"
#include "ui/ResultCard.h"
#include "ui/SelectionDiagnostics.h"
class QPlainTextEdit;
class QLabel;
namespace ws {
class Application : public QObject {
    Q_OBJECT
public:
    Application(AppController& controller, ISecretStore& secrets, QSettings& settings, ISelectionMonitor* monitor = nullptr);
    ~Application() override;
    void start();
    void openSettings();
    void openDiagnostics();
    bool smokeCheck();
private:
    void refreshDiagnostics();
    ISelectionMonitor* monitor_;
    SelectionDiagnostics* selectionPanel_;
    AppController& controller_;
    ISecretStore& secrets_;
    QSettings& settingsStore_;
    WorkspaceWindow workspace_;
    SettingsWindow settings_;
    ResultCard result_;
    QWidget diagnostics_;
    QPlainTextEdit* diagnosticsText_;
    QLabel* testStatus_;
    QString keyStatus_ = "Not checked";
    QMenu menu_;
    QSystemTrayIcon tray_;
};
}
