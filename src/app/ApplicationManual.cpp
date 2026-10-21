#include "Application.h"
#include <QAction>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QPointer>
namespace ws {
void Application::setupManual() {
    auto* trigger=menu_.addAction("Open Selection Actions",this,&Application::triggerManualActions);
    trigger->setObjectName("openSelectionActions");
    shortcutAction_=menu_.addAction("Enable Global Shortcut");
    shortcutAction_->setObjectName("enableGlobalShortcut"); shortcutAction_->setCheckable(true);
    shortcutAction_->setChecked(settingsStore_.value("shortcut/enabled",false).toBool());
    connect(shortcutAction_,&QAction::toggled,this,&Application::setShortcutEnabled);
    connect(&settings_,&SettingsWindow::globalShortcutChanged,this,&Application::setShortcutEnabled);
    connect(&settings_,&SettingsWindow::configureShortcutRequested,this,[this]{setShortcutEnabled(true);});
    connect(&manualPalette_,&ManualActionPalette::featureChosen,this,[this](Feature feature,const Selection& value){
        if(!controller_.busy()) controller_.runSelection(feature,value);
    });
    connect(&controller_,&AppController::loading,this,[this]{++manualGeneration_;if(resolver_)resolver_->cancel();manualPalette_.hide();});
    if(shortcut_) {
        connect(shortcut_,&IGlobalShortcut::activated,this,&Application::triggerManualActions);
        connect(shortcut_,&IGlobalShortcut::statusChanged,this,&Application::refreshManualDiagnostics);
    }
    manualText_=new QPlainTextEdit(&diagnostics_); manualText_->setObjectName("manualMetadata");
    manualText_->setReadOnly(true); manualText_->setMaximumHeight(180);
    qobject_cast<QVBoxLayout*>(diagnostics_.layout())->insertWidget(2,manualText_);
    refreshManualDiagnostics();
}
void Application::triggerManualActions() {
    actionBar_.hideBar();
    const auto generation=++manualGeneration_;
    if(resolver_) resolver_->cancel();
    manualPalette_.hide();
    if(controller_.busy()) {
        manualPalette_.showMessage("AI request in progress. Finish or cancel it before choosing another action.");
        return;
    }
    QPointer<Application> guard(this);
    auto completed=[guard,generation](std::optional<ResolvedSelection> resolved){
        if(!guard || guard->manualGeneration_!=generation || guard->controller_.busy())return;
        guard->lastManualSource_=resolved?selectionSourceName(resolved->source):"none";
        guard->lastManualCharacters_=resolved?resolved->selection.text.size():0;
        guard->lastManualTrigger_=QDateTime::currentDateTimeUtc();
        if(resolved) guard->manualPalette_.showSelection(*resolved);
        else guard->manualPalette_.showMessage("No selected text found.\nSelect text again and trigger, or copy text first.\nMaximum: 100,000 characters.");
        guard->refreshManualDiagnostics();
    };
    if(resolver_)resolver_->resolveAsync(this,std::move(completed));else completed({});
}
void Application::setShortcutEnabled(bool enabled) {
    settingsStore_.setValue("shortcut/enabled",enabled); settingsStore_.sync();
    {QSignalBlocker block(shortcutAction_);shortcutAction_->setChecked(enabled);}
    settings_.setGlobalShortcutChecked(enabled);
    if(shortcut_) {if(enabled) shortcut_->start(true);else shortcut_->stop();}
    refreshManualDiagnostics();
}
void Application::refreshManualDiagnostics() {
    if(!manualText_) return;
    const auto s=shortcut_?shortcut_->status():GlobalShortcutStatus{};
    const auto text=QString("Global shortcut preference: %1\nBackend: %2\nAvailable: %3; Registered: %4\n"
        "Preferred: %5; Actual: %6\nPortal version: %7\nStatus / last error: %8\nLast activation: %9\n"
        "Native selection cache: %10; PRIMARY supported: %11\nClipboard: explicit fallback only (not read for diagnostics)\n"
        "Last manual resolution: source=%12 characters=%13 at=%14\nExternal trigger: worksidekick --trigger; tray trigger available\nResolution order: %15")
        .arg(settingsStore_.value("shortcut/enabled",false).toBool()?"enabled":"disabled",
             s.backend,s.available?"yes":"no",s.registered?"yes":"no",PreferredShortcut,s.triggerDescription.isEmpty()?"none":s.triggerDescription,
             s.portalVersion?QString::number(s.portalVersion):"n/a",s.description,s.lastActivation.toString(Qt::ISODate),
             monitor_ && monitor_->latestSelection()?"available":"none",resolver_ && resolver_->primarySupported()?"yes":"no",
             lastManualSource_,QString::number(lastManualCharacters_),lastManualTrigger_.toString(Qt::ISODate),
             resolver_?resolver_->description():QString("Unavailable"));
    if(manualText_->toPlainText()!=text) manualText_->setPlainText(text);
}
}
