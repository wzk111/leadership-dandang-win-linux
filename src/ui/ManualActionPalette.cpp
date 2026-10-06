#include "ManualActionPalette.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
namespace ws {
ManualActionPalette::ManualActionPalette() {
    setWindowTitle("WorkSidekick — Selection Actions"); resize(540,300);
    auto* layout=new QVBoxLayout(this);
    source_=new QLabel(this); source_->setTextFormat(Qt::PlainText); source_->setWordWrap(true);
    source_->setObjectName("manualSource"); layout->addWidget(source_);
    preview_=new QPlainTextEdit(this); preview_->setObjectName("manualPreview"); preview_->setReadOnly(true);
    layout->addWidget(preview_);
    auto* row=new QHBoxLayout;
    for(const auto& f:FeatureRegistry::all()) {
        auto* b=new QPushButton(f.name,this); b->setObjectName(QString("manualAction%1").arg(int(f.id)));
        b->setToolTip(f.description); b->setEnabled(false); buttons_<<b; row->addWidget(b);
        connect(b,&QPushButton::clicked,this,[this,id=f.id] {
            if(!isVisible() || !snapshot_) return;
            const Selection value=snapshot_->selection;
            clear(); hide(); emit featureChosen(id,value);
        });
    }
    layout->addLayout(row);
    auto* closeButton=new QPushButton("Close",this); layout->addWidget(closeButton);
    connect(closeButton,&QPushButton::clicked,this,&QWidget::close);
}
void ManualActionPalette::showSelection(const ResolvedSelection& selection) {
    if(selection.selection.text.trimmed().isEmpty() || selection.selection.text.size()>100000) {
        showMessage("No valid text available (maximum 100,000 characters)."); return;
    }
    snapshot_=selection;
    source_->setText(QString("Source: %1 · %2 characters\nReview this local preview before choosing an AI action.")
        .arg(selectionSourceName(selection.source)).arg(selection.selection.text.size()));
    preview_->setPlainText(selection.selection.text.left(1000)+(selection.selection.text.size()>1000?"\n… (preview truncated)":""));
    for(auto* b:buttons_) b->setEnabled(true);
    show(); raise(); activateWindow();
}
void ManualActionPalette::showMessage(const QString& message) {
    clear(); source_->setText(message); show(); raise(); activateWindow();
}
void ManualActionPalette::clear() {
    snapshot_.reset(); preview_->clear(); source_->clear();
    for(auto* b:buttons_) b->setEnabled(false);
}
void ManualActionPalette::hideEvent(QHideEvent* e) { clear(); QWidget::hideEvent(e); }
}
