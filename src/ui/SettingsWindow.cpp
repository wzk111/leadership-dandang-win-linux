#include "SettingsWindow.h"
#include "core/Core.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QCloseEvent>
namespace ws {
SettingsWindow::SettingsWindow(QSettings& settings, ISecretStore& secrets, QWidget* parent)
    : QWidget(parent), settings_(settings), secrets_(secrets) {
    setWindowTitle("WorkSidekick — Settings"); resize(540, 360);
    const auto initial = Settings::load(settings_);
    auto* layout = new QVBoxLayout(this); auto* form = new QFormLayout;
    auto* provider = new QLabel("OpenAI — official Responses API", this);
    model_ = new QLineEdit(initial.model, this); model_->setObjectName("model");
    model_->setPlaceholderText("Enter a model ID available to your API project"); model_->setMaxLength(200);
    language_ = new QComboBox(this); language_->setObjectName("outputLanguage"); language_->setEditable(true);
    language_->addItems({"Same as input", "English", "中文", "日本語"}); language_->setCurrentText(initial.outputLanguage);
    language_->lineEdit()->setMaxLength(100);
    key_ = new QLineEdit(this); key_->setObjectName("apiKey"); key_->setEchoMode(QLineEdit::Password);
    key_->setMaxLength(4096); key_->setPlaceholderText("Enter a new key to save securely");
    form->addRow("Provider", provider); form->addRow("Model", model_); form->addRow("Output language", language_);
    form->addRow("API key", key_); layout->addLayout(form);
    auto* keyRow = new QHBoxLayout;
    saveKey_ = new QPushButton("Save API key securely", this); saveKey_->setObjectName("saveKey");
    removeKey_ = new QPushButton("Delete saved key", this); removeKey_->setObjectName("deleteKey");
    keyRow->addWidget(saveKey_); keyRow->addWidget(removeKey_); layout->addLayout(keyRow);
    auto* info = new QLabel("Keys are stored in your OS keyring, never in settings.\nAn OpenAI API account and API billing are required.", this); info->setWordWrap(true);
    layout->addWidget(info);
    status_ = new QLabel("Key status: not checked. Existing keys are never displayed.", this);
    status_->setTextFormat(Qt::PlainText); status_->setWordWrap(true); layout->addWidget(status_);
    auto* save = new QPushButton("Save settings", this); save->setObjectName("saveSettings"); layout->addWidget(save);
    connect(save, &QPushButton::clicked, this, [this] {
        if (model_->text().trimmed().isEmpty()) { status_->setText("Enter a model ID first."); return; }
        const bool ok = Settings{model_->text(), language_->currentText()}.save(settings_);
        status_->setText(ok ? "Settings saved." : "Could not save settings. Check local file permissions.");
    });
    connect(saveKey_, &QPushButton::clicked, this, [this] {
        auto key = key_->text().trimmed(); key_->clear();
        if (key.isEmpty()) { status_->setText("Enter a key first."); return; }
        if (!secrets_.save(key)) { status_->setText("Secret store is busy. Try again."); return; }
        saveKey_->setEnabled(false); removeKey_->setEnabled(false); status_->setText("Saving to system keyring…");
    });
    connect(removeKey_, &QPushButton::clicked, this, [this] {
        key_->clear();
        if (!secrets_.remove()) { status_->setText("Secret store is busy. Try again."); return; }
        saveKey_->setEnabled(false); removeKey_->setEnabled(false); status_->setText("Removing saved key…");
    });
    connect(&secrets_, &ISecretStore::writeFinished, this, [this](bool, const QString& message) {
        saveKey_->setEnabled(true); removeKey_->setEnabled(true); status_->setText(message);
    });
}
void SettingsWindow::closeEvent(QCloseEvent* event) {
    key_->clear(); QWidget::closeEvent(event);
}
}
