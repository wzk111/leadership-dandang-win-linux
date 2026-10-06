#include "ReplyComposer.h"
namespace ws {
ReplyComposer::ReplyComposer() {}
void ReplyComposer::openSelection(const Selection&) {}
void ReplyComposer::hideEvent(QHideEvent* e) {QWidget::hideEvent(e);}
}
