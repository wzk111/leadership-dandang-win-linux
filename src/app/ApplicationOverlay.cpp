#include "Application.h"
#include <QAction>
#include <QGuiApplication>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace ws {
void Application::setupOverlay() {
    policyConfigured_ = windowPolicy_ && windowPolicy_->configureActionBar(actionBar_);
    overlayText_ = new QPlainTextEdit(&diagnostics_);
    overlayText_->setObjectName("overlayMetadata"); overlayText_->setReadOnly(true);
    overlayText_->setMaximumHeight(170);
    qobject_cast<QVBoxLayout*>(diagnostics_.layout())->insertWidget(1, overlayText_);
    automaticAction_ = menu_.addAction("Enable Automatic Toolbar");
    automaticAction_->setObjectName("automaticToolbar"); automaticAction_->setCheckable(true);
    automaticAction_->setChecked(settingsStore_.value("ui/automaticPopup", false).toBool());
    connect(automaticAction_, &QAction::toggled, this, &Application::setAutomaticPopup);
    connect(&settings_, &SettingsWindow::automaticPopupChanged, this, &Application::setAutomaticPopup);
    connect(&actionBar_, &ActionBar::dismissed, this, [this] { overlayStatus_="Dismissed"; refreshOverlayDiagnostics(); });
    connect(&actionBar_, &ActionBar::featureChosen, this, [this](Feature feature, const Selection& snapshot) {
        // The value was copied before hiding. Never query clipboard or monitor here.
        if (!controller_.busy()) controller_.runSelection(feature, snapshot);
        overlayStatus_="Explicit action chosen"; refreshOverlayDiagnostics();
    });
    connect(&controller_, &AppController::loading, this, [this] {
        actionBar_.hideBar(); overlayStatus_="Hidden while AI is busy"; refreshOverlayDiagnostics();
    });
    if (monitor_) {
        connect(monitor_, &ISelectionMonitor::selectionDetected, this, &Application::showSelectionToolbar);
        connect(monitor_, &ISelectionMonitor::selectionCleared, this, [this] {
            actionBar_.hideBar(); overlayStatus_="Selection cleared / expired"; refreshOverlayDiagnostics();
        });
        connect(monitor_, &ISelectionMonitor::statusChanged, this, [this] {
            if (!monitor_->status().running) {
                actionBar_.hideBar(); overlayStatus_="Monitor stopped / unavailable";
                refreshOverlayDiagnostics();
            }
        });
    }
    refreshOverlayDiagnostics();
}
void Application::setAutomaticPopup(bool enabled) {
    settingsStore_.setValue("ui/automaticPopup", enabled); settingsStore_.sync();
    { QSignalBlocker block(automaticAction_); automaticAction_->setChecked(enabled); }
    settings_.setAutomaticPopupChecked(enabled);
    if (!enabled) actionBar_.hideBar();
    overlayStatus_ = enabled ? "Enabled; waiting for a new selection" : "Disabled";
    if (settingsStore_.status() != QSettings::NoError)
        overlayStatus_ += "; preference could not be saved";
    refreshOverlayDiagnostics();
}
void Application::showSelectionToolbar(const Selection& selection) {
    lastAnchor_ = selection.anchorRect;
    auto hide = [this](const QString& why) { actionBar_.hideBar(); overlayStatus_=why; refreshOverlayDiagnostics(); };
    if (!settingsStore_.value("ui/automaticPopup", false).toBool()) { hide("Disabled"); return; }
    if (!policyConfigured_ || !windowPolicy_ ||
        windowPolicy_->overlayCapability() != OverlayCapability::AnchoredNonActivating) {
        hide("Automatic toolbar inactive: unsupported backend; manual clipboard mode available"); return;
    }
    if (!monitor_ || !monitor_->status().running) { hide("Monitor stopped / unavailable"); return; }
    if (controller_.busy()) { hide("Hidden while AI is busy"); return; }
    if (selection.text.trimmed().isEmpty()) { hide("Empty selection"); return; }
    if (!selection.anchorRect || !selection.anchorRect->isValid()) {
        hide("Selection detected but automatic ActionBar unavailable: no anchor rectangle"); return;
    }
    const auto placement = windowPolicy_->positionActionBar(actionBar_, *selection.anchorRect);
    if (!placement) { hide("Automatic ActionBar unavailable: no usable screen placement"); return; }
    lastPlacement_ = placement;
    actionBar_.showForSelection(selection, placement->geometry.topLeft());
    overlayStatus_="Shown locally; AI requires an explicit feature click";
    refreshOverlayDiagnostics();
}
void Application::refreshOverlayDiagnostics() {
    if (!overlayText_) return;
    const auto rect = [](const std::optional<QRect>& r) {
        return r ? QString("x=%1 y=%2 w=%3 h=%4").arg(r->x()).arg(r->y()).arg(r->width()).arg(r->height()) : QString("none");
    };
    const auto text = QString("Automatic toolbar preference: %1\nOverlay capability: %2\nQt platform: %3\n"
        "Status: %4\nLast selection anchor (raw AT-SPI): %5\nLast toolbar placement (requested Qt): %6\n"
        "Placement mode: %7\nSource focus preserved on this desktop: NOT TESTED\nHiDPI mapping: NOT TESTED")
        .arg(settingsStore_.value("ui/automaticPopup",false).toBool() ? "enabled" : "disabled",
             windowPolicy_ ? windowPolicy_->description() : "Unsupported: no platform window policy",
             QGuiApplication::platformName(), overlayStatus_, rect(lastAnchor_),
             rect(lastPlacement_ ? std::optional<QRect>(lastPlacement_->geometry) : std::nullopt),
             lastPlacement_ ? lastPlacement_->mode : "none");
    if (overlayText_->toPlainText() != text) overlayText_->setPlainText(text);
}
}
