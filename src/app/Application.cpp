#include "Application.h"
#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QSysInfo>
namespace ws {
Application::Application(AppController& controller, ISecretStore& secrets, QSettings& settings, ISelectionMonitor* monitor)
    : monitor_(monitor), controller_(controller), secrets_(secrets), settingsStore_(settings), settings_(settings, secrets) {
    auto showWorkspace = [this] { workspace_.show(); workspace_.raise(); workspace_.activateWindow(); };
    connect(&workspace_, &WorkspaceWindow::processClipboard, &controller_, &AppController::captureClipboard);
    connect(&workspace_, &WorkspaceWindow::featureChosen, &controller_, &AppController::run);
    connect(&workspace_, &WorkspaceWindow::settingsRequested, this, &Application::openSettings);
    connect(&workspace_, &WorkspaceWindow::diagnosticsRequested, this, &Application::openDiagnostics);
    connect(&controller_, &AppController::captured, &workspace_, &WorkspaceWindow::setCaptured);
    connect(&controller_, &AppController::loading, this, [this] { workspace_.setBusy(true); result_.loading(); });
    connect(&controller_, &AppController::finished, this, [this](const AIResult& result) {
        workspace_.setBusy(false);
        if (result.error == AIError::None) result_.success(result.text);
        else result_.error(result.message);
    });
    connect(&result_, &ResultCard::cancelRequested, &controller_, &AppController::cancel);
    connect(qApp, &QApplication::aboutToQuit, &controller_, &AppController::cancel);
    menu_.addAction("Open WorkSidekick", this, showWorkspace);
    menu_.addAction("Process Clipboard", this, [this, showWorkspace] { controller_.captureClipboard(); showWorkspace(); });
    menu_.addSeparator();
    menu_.addAction("Settings", this, &Application::openSettings);
    menu_.addAction("Diagnostics", this, &Application::openDiagnostics);
    menu_.addAction("About", this, [this] {
        QMessageBox::about(&workspace_, "WorkSidekick", "WorkSidekick 0.2 — M1\nExplicit clipboard AI assistant.\nYou review and copy; nothing is sent automatically.");
    });
    menu_.addSeparator(); menu_.addAction("Quit", qApp, &QApplication::quit);
    tray_.setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    tray_.setToolTip("WorkSidekick"); tray_.setContextMenu(&menu_);
    connect(&tray_, &QSystemTrayIcon::activated, this, [showWorkspace](auto reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) showWorkspace();
    });
    diagnostics_.setWindowTitle("WorkSidekick — Diagnostics"); diagnostics_.resize(700, 760);
    auto* layout = new QVBoxLayout(&diagnostics_);
    diagnosticsText_ = new QPlainTextEdit(&diagnostics_); diagnosticsText_->setReadOnly(true); layout->addWidget(diagnosticsText_);
    testStatus_ = new QLabel("Metadata excludes selected text; local preview requires an explicit action.", &diagnostics_);
    testStatus_->setWordWrap(true); layout->addWidget(testStatus_);
    selectionPanel_ = new SelectionDiagnostics(monitor_, &diagnostics_);
    layout->addWidget(selectionPanel_);
    auto* row = new QHBoxLayout;
    auto* selection = new QPushButton("Test Selection", &diagnostics_);
    auto* clipboard = new QPushButton("Test Clipboard", &diagnostics_);
    auto* ai = new QPushButton("Test AI Connection", &diagnostics_);
    auto* refresh = new QPushButton("Refresh", &diagnostics_);
    row->addWidget(selection); row->addWidget(clipboard); row->addWidget(ai); row->addWidget(refresh); layout->addLayout(row);
    connect(selection, &QPushButton::clicked, this, [this] { selectionPanel_->refresh(); });
    connect(clipboard, &QPushButton::clicked, this, [this] {
        const auto text = QApplication::clipboard()->text();
        testStatus_->setText(text.trimmed().isEmpty() ? "Clipboard has no text." : QString("Clipboard text available: %1 characters. No API request made.").arg(text.size()));
    });
    connect(ai, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(&diagnostics_, "Test AI Connection",
            "Send a short synthetic test message to OpenAI? API charges may apply. Your clipboard is not used.") == QMessageBox::Yes)
            controller_.testConnection();
    });
    connect(refresh, &QPushButton::clicked, this, &Application::refreshDiagnostics);
    connect(&secrets_, &ISecretStore::readFinished, this, [this](const SecretResult& r) {
        keyStatus_ = !r.error.isEmpty() ? "Unavailable / locked" : r.key.isEmpty() ? "No key saved" : "Key available";
        refreshDiagnostics();
    });
    connect(&secrets_, &ISecretStore::writeFinished, this, [this](bool ok, const QString&) {
        keyStatus_ = ok ? "Storage operation succeeded; key presence checked on next request" : "Unavailable / locked";
        refreshDiagnostics();
    });
}
Application::~Application() { if (monitor_) monitor_->stop(); }
void Application::start() {
    if (monitor_) monitor_->start();
    const bool available = QSystemTrayIcon::isSystemTrayAvailable();
    QApplication::setQuitOnLastWindowClosed(!available);
    workspace_.setTrayAvailable(available);
    if (available) tray_.show();
    workspace_.show();
}
void Application::openSettings() { settings_.show(); settings_.raise(); settings_.activateWindow(); }
void Application::openDiagnostics() { refreshDiagnostics(); diagnostics_.show(); diagnostics_.raise(); }
void Application::refreshDiagnostics() {
    diagnosticsText_->setPlainText(QString(
        "WorkSidekick 0.2 — M1\nOS: %1\nDesktop: %2\nXDG_SESSION_TYPE: %3\nQt: %4\nQt platform: %5\n"
        "Selection mode: MANUAL CLIPBOARD\nAT-SPI runtime status: see local selection panel below\n"
        "X11 selection backend: not implemented (M3)\nGlobalShortcuts portal: not probed (M3)\n"
        "Registered shortcut: none (M3)\nAutomatic popup: disabled (M2)\n"
        "Secret backend: %6\nLast key status: %7\nAI provider: OpenAI Responses\nModel configured: %8\n"
        "System tray detected: %9\nEndpoint: https://api.openai.com/v1/responses\nTimeout: 30 seconds")
        .arg(QSysInfo::prettyProductName(), qEnvironmentVariable("XDG_CURRENT_DESKTOP", "unknown"),
             qEnvironmentVariable("XDG_SESSION_TYPE", "unknown"), qVersion(), QGuiApplication::platformName(),
             secrets_.description(), keyStatus_, Settings::load(settingsStore_).model.isEmpty() ? "no" : "yes",
             QSystemTrayIcon::isSystemTrayAvailable() ? "yes" : "no"));
}
bool Application::smokeCheck() {
    openSettings(); openDiagnostics(); result_.loading(); result_.success("Synthetic smoke test.");
    return workspace_.isVisible() && settings_.isVisible() && diagnostics_.isVisible() && result_.isVisible();
}
}
