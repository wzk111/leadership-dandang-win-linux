#include "ResultCard.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
namespace ws {
ResultCard::ResultCard(QWidget* parent) : QWidget(parent) {
    setWindowTitle("WorkSidekick — Result"); resize(560, 400);
    auto* layout = new QVBoxLayout(this);
    status_ = new QLabel("Ready", this); status_->setTextFormat(Qt::PlainText);
    text_ = new QPlainTextEdit(this); text_->setObjectName("resultText"); text_->setReadOnly(true);
    text_->setPlaceholderText("Your result will appear here.");
    auto* row = new QHBoxLayout;
    copy_ = new QPushButton("Copy result", this); copy_->setObjectName("copyResult"); copy_->setEnabled(false);
    cancel_ = new QPushButton("Cancel", this); cancel_->setObjectName("cancelRequest"); cancel_->setEnabled(false);
    auto* close = new QPushButton("Close", this);
    row->addWidget(copy_); row->addWidget(cancel_); row->addStretch(); row->addWidget(close);
    layout->addWidget(status_); layout->addWidget(text_); layout->addLayout(row);
    connect(copy_, &QPushButton::clicked, this, [this] {
        if (copy_->isEnabled()) { QApplication::clipboard()->setText(text_->toPlainText()); status_->setText("Copied. You decide where to paste it."); }
    });
    connect(cancel_, &QPushButton::clicked, this, &ResultCard::cancelRequested);
    connect(close, &QPushButton::clicked, this, &QWidget::close);
}
void ResultCard::loading() {
    text_->clear(); status_->setText("Working…"); copy_->setEnabled(false); cancel_->setEnabled(true);
    show(); raise();
}
void ResultCard::success(const QString& text) {
    text_->setPlainText(text); status_->setText("Ready — review before using.");
    copy_->setEnabled(!text.isEmpty()); cancel_->setEnabled(false); show();
}
void ResultCard::error(const QString& message) {
    text_->clear(); status_->setText(message); copy_->setEnabled(false); cancel_->setEnabled(false); show();
}
void ResultCard::closeEvent(QCloseEvent* event) {
    if (cancel_->isEnabled()) emit cancelRequested();
    text_->clear(); copy_->setEnabled(false); cancel_->setEnabled(false);
    QWidget::closeEvent(event);
}
}
