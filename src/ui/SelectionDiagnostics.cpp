#include "SelectionDiagnostics.h"
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHideEvent>
namespace ws {
SelectionDiagnostics::SelectionDiagnostics(ISelectionMonitor* monitor, QWidget* parent)
    : QWidget(parent), monitor_(monitor) {
    setObjectName("selectionDiagnostics");
    auto* layout=new QVBoxLayout(this);
    metadata_=new QPlainTextEdit(this); metadata_->setObjectName("selectionMetadata");
    metadata_->setReadOnly(true); metadata_->setMaximumHeight(180); layout->addWidget(metadata_);
    auto* row=new QHBoxLayout;
    auto* preview=new QPushButton("Show Last Selection Preview",this); preview->setObjectName("showSelectionPreview");
    auto* start=new QPushButton("Start monitoring",this);
    auto* stop=new QPushButton("Stop monitoring",this);
    row->addWidget(preview); row->addWidget(start); row->addWidget(stop); layout->addLayout(row);
    layout->addWidget(new QLabel("Local diagnostic preview — not sent to AI",this));
    preview_=new QPlainTextEdit(this); preview_->setObjectName("selectionPreview");
    preview_->setReadOnly(true); preview_->setMaximumHeight(100); preview_->hide(); layout->addWidget(preview_);
    connect(preview,&QPushButton::clicked,this,[this] {
        preview_->clear();
        const auto last=monitor_ ? monitor_->latestSelection() : std::nullopt;
        if(last) { preview_->setPlainText(last->text); preview_->show(); }
        else refresh();
    });
    if(monitor_) {
        connect(start,&QPushButton::clicked,monitor_,&ISelectionMonitor::start);
        connect(stop,&QPushButton::clicked,monitor_,&ISelectionMonitor::stop);
        connect(monitor_,&ISelectionMonitor::statusChanged,this,&SelectionDiagnostics::refresh);
        const auto clear=[this] { preview_->clear(); preview_->hide(); };
        connect(monitor_,&ISelectionMonitor::selectionDetected,this,clear);
        connect(monitor_,&ISelectionMonitor::selectionCleared,this,clear);
    } else { start->setEnabled(false); stop->setEnabled(false); preview->setEnabled(false); }
    refresh();
}
void SelectionDiagnostics::refresh() {
    if(!monitor_) { metadata_->setPlainText("AT-SPI monitor not attached (test/shared build)."); return; }
    const auto s=monitor_->status(); const auto last=monitor_->latestSelection();
    QString text=QString("Backend: %1\nAT-SPI available: %2; initialized: %3; listener running: %4\n%5\nEvents received: %6\nLast event (UTC): %7\nLast result: %8\nAI triggered automatically: NO\nSelection mode for AI: MANUAL CLIPBOARD")
        .arg(monitor_->backendName(),s.available?"yes":"no",s.initialized?"yes":"no",s.running?"yes":"no",
             s.description,QString::number(s.events),s.lastEvent.isValid()?s.lastEvent.toString(Qt::ISODate):"none",s.lastResult);
    if(last) {
        text+=QString("\nApplication: %1\nCharacters: %2\nText retrieved: yes\nAnchor available: %3")
            .arg(last->sourceApplication).arg(last->text.size()).arg(last->anchorRect?"yes":"no");
        if(last->anchorRect) {
            const auto r=*last->anchorRect;
            text+=QString("\nAT-SPI screen coordinates: x=%1 y=%2 w=%3 h=%4").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
        }
    } else text+="\nNo AT-SPI text selection detected yet. Select text in another accessible application and try again.";
    // Avoid resetting accessible text/cursor for identical metadata.
    if(metadata_->toPlainText()!=text) metadata_->setPlainText(text);
}
void SelectionDiagnostics::hideEvent(QHideEvent* event) {
    preview_->clear(); preview_->hide(); QWidget::hideEvent(event);
}
}
