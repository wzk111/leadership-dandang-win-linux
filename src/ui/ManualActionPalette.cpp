#include "ManualActionPalette.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
namespace ws {
ManualActionPalette::ManualActionPalette() {
    auto* layout=new QVBoxLayout(this);
    source_=new QLabel(this); source_->setObjectName("manualSource"); layout->addWidget(source_);
    preview_=new QPlainTextEdit(this); preview_->setObjectName("manualPreview"); layout->addWidget(preview_);
    for(const auto& f:FeatureRegistry::all()) {
        auto* b=new QPushButton(f.name,this); b->setObjectName(QString("manualAction%1").arg(int(f.id)));
        buttons_<<b; layout->addWidget(b);
    }
}
void ManualActionPalette::showSelection(const ResolvedSelection&) {}
void ManualActionPalette::showMessage(const QString&) {}
void ManualActionPalette::clear() {}
void ManualActionPalette::hideEvent(QHideEvent* e) { QWidget::hideEvent(e); }
}
