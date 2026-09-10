#include "WorkspaceWindow.h"
#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
namespace ws {
WorkspaceWindow::WorkspaceWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("WorkSidekick"); resize(610, 460);
    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("WorkSidekick", this);
    QFont font = title->font(); font.setPointSize(20); font.setBold(true); title->setFont(font);
    auto* help = new QLabel("Copy text in another app, load it here, then choose an action.\nOnly the text shown below is sent when you choose an action.", this);
    help->setWordWrap(true);
    trayNotice_ = new QLabel(this); trayNotice_->setWordWrap(true);
    process_ = new QPushButton("Process Clipboard", this); process_->setObjectName("processClipboard");
    count_ = new QLabel("No text loaded", this);
    preview_ = new QPlainTextEdit(this); preview_->setObjectName("clipboardPreview"); preview_->setReadOnly(true);
    preview_->setPlaceholderText("Nothing is read until you click Process Clipboard.");
    auto* actions = new QHBoxLayout;
    for (const auto& f : FeatureRegistry::all()) {
        auto* button = new QPushButton(f.name, this);
        button->setToolTip(f.description); button->setEnabled(false);
        button->setObjectName("feature" + QString::number(static_cast<int>(f.id)));
        features_.append(button); actions->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, id = f.id] { emit featureChosen(id); });
    }
    auto* footer = new QHBoxLayout;
    auto* settings = new QPushButton("Settings", this); settings->setObjectName("openSettings");
    auto* diagnostics = new QPushButton("Diagnostics", this); diagnostics->setObjectName("openDiagnostics");
    auto* quit = new QPushButton("Quit", this);
    footer->addWidget(settings); footer->addWidget(diagnostics); footer->addStretch(); footer->addWidget(quit);
    layout->addWidget(title); layout->addWidget(help); layout->addWidget(trayNotice_);
    layout->addWidget(process_); layout->addWidget(count_); layout->addWidget(preview_);
    layout->addLayout(actions); layout->addLayout(footer);
    connect(process_, &QPushButton::clicked, this, &WorkspaceWindow::processClipboard);
    connect(settings, &QPushButton::clicked, this, &WorkspaceWindow::settingsRequested);
    connect(diagnostics, &QPushButton::clicked, this, &WorkspaceWindow::diagnosticsRequested);
    connect(quit, &QPushButton::clicked, qApp, &QApplication::quit);
}
void WorkspaceWindow::setCaptured(const QString& text) {
    preview_->setPlainText(text); count_->setText(QString("%1 characters loaded").arg(text.size()));
    setBusy(false);
}
void WorkspaceWindow::setBusy(bool busy) {
    process_->setEnabled(!busy);
    for (auto* button : features_) button->setEnabled(!busy && !preview_->toPlainText().trimmed().isEmpty());
}
void WorkspaceWindow::setTrayAvailable(bool available) {
    trayNotice_->setText(available ? "Closing a window keeps WorkSidekick in the tray. Use Quit to exit."
        : "No system tray detected. Keep this window open; closing the last window exits.");
}
}
