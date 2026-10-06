#pragma once
#include <QObject>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QWidget>
#include "AppController.h"
#include "platform/IPlatformWindowPolicy.h"
#include "ui/WorkspaceWindow.h"
#include "ui/SettingsWindow.h"
#include "ui/ResultCard.h"
#include "ui/SelectionDiagnostics.h"
#include "ui/ActionBar.h"
class QPlainTextEdit;
class QLabel;
namespace ws {
class Application : public QObject {
    Q_OBJECT
public:
    Application(AppController& controller, ISecretStore& secrets, QSettings& settings, ISelectionMonitor* monitor = nullptr, IPlatformWindowPolicy* policy = nullptr);
    ~Application() override;
    void start();
    void openSettings();
    void openDiagnostics();
    bool smokeCheck();
private:
    void refreshDiagnostics();
    void setupOverlay();
    void setAutomaticPopup(bool enabled);
    void showSelectionToolbar(const Selection& selection);
    void refreshOverlayDiagnostics();
    IPlatformWindowPolicy* windowPolicy_;
    ActionBar actionBar_;
    QAction* automaticAction_ = nullptr;
    QPlainTextEdit* overlayText_ = nullptr;
    bool policyConfigured_ = false;
    std::optional<QRect> lastAnchor_;
    std::optional<BarPlacement> lastPlacement_;
    QString overlayStatus_ = "Waiting for selection";
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
