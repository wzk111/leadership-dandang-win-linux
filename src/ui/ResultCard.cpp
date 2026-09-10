#include "ResultCard.h"
#include <QCloseEvent>
namespace ws {
ResultCard::ResultCard(QWidget* parent) : QWidget(parent) {}
void ResultCard::loading() {}
void ResultCard::success(const QString&) {}
void ResultCard::error(const QString&) {}
void ResultCard::closeEvent(QCloseEvent* event) { QWidget::closeEvent(event); }
}
