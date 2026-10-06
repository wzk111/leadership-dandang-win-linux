#include "ReplyComposer.h"
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
namespace ws {
ReplyComposer::ReplyComposer() {
    setWindowTitle("WorkSidekick — Reply");resize(570,430);
    auto* layout=new QVBoxLayout(this);
    auto* help=new QLabel("Review the incoming message and choose what to communicate.\nNothing is uploaded until Generate Reply.",this);
    help->setWordWrap(true);layout->addWidget(help);
    preview_=new QPlainTextEdit(this);preview_->setReadOnly(true);preview_->setObjectName("replySource");
    preview_->setAccessibleName("Incoming message preview");layout->addWidget(preview_);
    auto* form=new QFormLayout;stance_=new QComboBox(this);stance_->setObjectName("replyStance");
    stance_->addItems(FeatureRegistry::info(Feature::Reply).supportedVariants);stance_->setCurrentText("Professional");
    form->addRow("Stance",stance_);layout->addLayout(form);
    intent_=new QPlainTextEdit(this);intent_->setObjectName("replyIntent");intent_->setAccessibleName("What do you want to communicate?");
    intent_->setPlaceholderText("Optional instruction (maximum 2,000 characters)");intent_->setMaximumHeight(95);layout->addWidget(intent_);
    auto* row=new QHBoxLayout;auto* generate=new QPushButton("Generate Reply",this);generate->setObjectName("generateReply");
    auto* cancel=new QPushButton("Cancel",this);row->addWidget(generate);row->addWidget(cancel);layout->addLayout(row);
    connect(intent_,&QPlainTextEdit::textChanged,this,[this,generate]{generate->setEnabled(intent_->toPlainText().size()<=2000);});
    connect(cancel,&QPushButton::clicked,this,&QWidget::close);
    connect(generate,&QPushButton::clicked,this,[this]{
        if(!selection_ || intent_->toPlainText().size()>2000)return;
        const auto value=*selection_;const auto stance=stance_->currentText();const auto intent=intent_->toPlainText();
        hide();emit generateRequested(value,stance,intent);
    });
}
void ReplyComposer::openSelection(const Selection& value) {
    if(value.text.trimmed().isEmpty() || value.text.size()>100000)return;
    selection_=value;preview_->setPlainText(value.text.left(1000)+(value.text.size()>1000?"\n… (preview truncated)":""));
    intent_->clear();stance_->setCurrentText("Professional");show();raise();activateWindow();
}
void ReplyComposer::hideEvent(QHideEvent* e) {selection_.reset();preview_->clear();intent_->clear();QWidget::hideEvent(e);}
}
