#include "ResultCard.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
namespace ws {
ResultCard::ResultCard(QWidget* parent):QWidget(parent) {
    setWindowTitle("WorkSidekick — Result");resize(600,430);auto* layout=new QVBoxLayout(this);
    status_=new QLabel("Ready",this);status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);
    metadata_=new QLabel(this);metadata_->setObjectName("resultMetadata");metadata_->setTextFormat(Qt::PlainText);
    text_=new QPlainTextEdit(this);text_->setObjectName("resultText");text_->setReadOnly(true);
    text_->setPlaceholderText("Your result will appear here.");
    auto* row=new QHBoxLayout;
    copy_=new QPushButton("Copy result",this);copy_->setObjectName("copyResult");copy_->setEnabled(false);
    cancel_=new QPushButton("Cancel",this);cancel_->setObjectName("cancelRequest");cancel_->setEnabled(false);
    regenerate_=new QPushButton("Regenerate",this);regenerate_->setObjectName("regenerate");regenerate_->setEnabled(false);
    adjustments_=new QToolButton(this);adjustments_->setText("Adjust");adjustments_->setAccessibleName("Adjust current result");
    adjustments_->setPopupMode(QToolButton::InstantPopup);auto* menu=new QMenu(adjustments_);
    for(const QString& label:{"Shorter","Longer","Friendlier","More Direct"}) {
        auto* action=menu->addAction(label);action->setObjectName("refine"+label);refinements_<<action;
        connect(action,&QAction::triggered,this,[this,label]{emit refinementRequested(label);});
    }
    adjustments_->setMenu(menu);adjustments_->setEnabled(false);
    auto* close=new QPushButton("Close",this);row->addWidget(copy_);row->addWidget(regenerate_);row->addWidget(adjustments_);
    row->addWidget(cancel_);row->addStretch();row->addWidget(close);
    layout->addWidget(status_);layout->addWidget(metadata_);layout->addWidget(text_);layout->addLayout(row);
    connect(copy_,&QPushButton::clicked,this,[this] {if(copy_->isEnabled()) {QApplication::clipboard()->setText(text_->toPlainText());status_->setText("Copied. You decide where to paste it.");}});
    connect(cancel_,&QPushButton::clicked,this,&ResultCard::cancelRequested);
    connect(close,&QPushButton::clicked,this,&QWidget::close);
    connect(regenerate_,&QPushButton::clicked,this,[this]{emit refinementRequested("Regenerate");});
}
void ResultCard::setRefinementEnabled(bool enabled) {
    regenerate_->setEnabled(enabled);adjustments_->setEnabled(enabled);for(auto* action:refinements_)action->setEnabled(enabled);
}
void ResultCard::setMetadata(const QString& text) {metadata_->setText(text);}
void ResultCard::loading(bool preserve) {
    if(!preserve) {text_->clear();metadata_->clear();}
    status_->setText("Working…");copy_->setEnabled(false);cancel_->setEnabled(true);setRefinementEnabled(false);show();raise();
}
void ResultCard::success(const QString& text) {
    text_->setPlainText(text);status_->setText("Ready — review before using.");copy_->setEnabled(!text.isEmpty());
    cancel_->setEnabled(false);setRefinementEnabled(!text.isEmpty());show();
}
void ResultCard::error(const QString& message,bool preserve) {
    if(!preserve)text_->clear();
    status_->setText(message);copy_->setEnabled(!text_->toPlainText().isEmpty());cancel_->setEnabled(false);
    setRefinementEnabled(!text_->toPlainText().isEmpty());show();
}
void ResultCard::closeEvent(QCloseEvent* event) {
    emit closed();
    if(cancel_->isEnabled())emit cancelRequested();
    text_->clear();metadata_->clear();copy_->setEnabled(false);cancel_->setEnabled(false);setRefinementEnabled(false);
    QWidget::closeEvent(event);
}
}
