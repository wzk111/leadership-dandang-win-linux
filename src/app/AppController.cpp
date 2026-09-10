#include "AppController.h"
namespace ws {
AppController::AppController(IAIProvider& ai, ISecretStore& secrets, ISelectionProvider& selection,
                             QSettings& settings, QObject* parent)
    : QObject(parent), ai_(ai), secrets_(secrets), selection_(selection), settings_(settings) {}
bool AppController::captureClipboard() { return false; }
bool AppController::run(Feature) { return false; }
void AppController::cancel() {}
bool AppController::busy() const { return false; }
}
