#include "ActionBar.h"
#include <QHBoxLayout>
#include <QPushButton>
namespace ws {
ActionBar::ActionBar(QWidget* parent) : QWidget(parent,
        Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus) {
    setObjectName("actionBar");
    setWindowTitle("WorkSidekick — Selection actions");
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_QuitOnClose, false);
    setFocusPolicy(Qt::NoFocus);
    setStyleSheet("#actionBar { background: #202832; border: 1px solid #526172; border-radius: 8px; }"
                  "QPushButton { color: #f4f7fa; background: transparent; border: none; padding: 7px 10px; border-radius: 4px; }"
                  "QPushButton:hover { background: #3e536b; } QPushButton:pressed { background: #526f8e; }");
    auto* row = new QHBoxLayout(this); row->setContentsMargins(4,4,4,4); row->setSpacing(2);
    for (const auto& feature : FeatureRegistry::all()) {
        auto* button = new QPushButton(feature.name, this);
        button->setObjectName(QString("action%1").arg(int(feature.id)));
        button->setToolTip(feature.description);
        button->setFocusPolicy(Qt::NoFocus);
        row->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, id=feature.id] {
            if (!isVisible() || !selection_) return;
            const Selection snapshot = *selection_;
            hideBar();
            emit featureChosen(id, snapshot);
        });
    }
    auto* dismiss = new QPushButton("×", this);
    dismiss->setObjectName("dismissActionBar"); dismiss->setAccessibleName("Dismiss selection toolbar");
    dismiss->setFocusPolicy(Qt::NoFocus); row->addWidget(dismiss);
    connect(dismiss, &QPushButton::clicked, this, [this] { hideBar(); emit dismissed(); });
    setFixedSize(sizeHint());
}
void ActionBar::showForSelection(const Selection& selection, const QPoint& position) {
    if (selection.text.trimmed().isEmpty()) { hideBar(); return; }
    selection_ = selection;
    move(position);
    if (!isVisible()) show();
}
void ActionBar::hideBar() { selection_.reset(); hide(); }
void ActionBar::hideEvent(QHideEvent* event) {
    selection_.reset();
    QWidget::hideEvent(event);
}
}
