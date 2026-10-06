#include "ActionBar.h"
#include <QHBoxLayout>
#include <QPushButton>
namespace ws {
ActionBar::ActionBar(QWidget* parent) : QWidget(parent) {
    auto* row = new QHBoxLayout(this);
    for (const auto& feature : FeatureRegistry::all()) {
        auto* button = new QPushButton(feature.name, this);
        button->setObjectName(QString("action%1").arg(int(feature.id)));
        row->addWidget(button);
    }
    auto* dismiss = new QPushButton("×", this); dismiss->setObjectName("dismissActionBar");
    row->addWidget(dismiss);
}
void ActionBar::showForSelection(const Selection&, const QPoint&) {}
void ActionBar::hideBar() { hide(); }
void ActionBar::hideEvent(QHideEvent* event) { QWidget::hideEvent(event); }
}
