#include "Application.h"
#include "core/FeaturePreferences.h"
#include "core/Profile.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
namespace ws {
void Application::setupProduct() {
    productText_ = new QPlainTextEdit(&diagnostics_);
    productText_->setObjectName("productMetadata"); productText_->setReadOnly(true);
    productText_->setMaximumHeight(160);
    qobject_cast<QVBoxLayout*>(diagnostics_.layout())->insertWidget(1,productText_);
    connect(&settings_,&SettingsWindow::preferencesChanged,this,[this] {
        refreshFeaturePreferences(); refreshDiagnostics();
    });
    connect(&settings_,&SettingsWindow::profileRequested,this,[this] {
        profile_.reload(); profile_.show(); profile_.raise(); profile_.activateWindow();
    });
    connect(&profile_,&ProfileWindow::profileChanged,this,&Application::refreshProductDiagnostics);
    connect(&settings_,&SettingsWindow::diagnosticsRequested,this,&Application::openDiagnostics);
    connect(&settings_,&SettingsWindow::testProviderRequested,&controller_,&AppController::testConnection);
    connect(&controller_,&AppController::loading,this,[this] { settings_.setRequestBusy(true); refreshProductDiagnostics(); });
    connect(&controller_,&AppController::finished,this,[this] { settings_.setRequestBusy(false); refreshProductDiagnostics(); });
    connect(&actionBar_,&ActionBar::moreRequested,this,[this](const Selection& snapshot) {
        if (!controller_.busy()) manualPalette_.showSelection({snapshot,monitor_?monitor_->selectionSource():SelectionSource::AtSpi});
    });
    refreshFeaturePreferences(); refreshProductDiagnostics();
}
void Application::refreshFeaturePreferences() {
    const auto preferences = FeaturePreferences::load(settingsStore_);
    actionBar_.setPreferences(preferences); manualPalette_.setPreferences(preferences); workspace_.setPreferences(preferences);
}
void Application::refreshProductDiagnostics() {
    if (!productText_) return;
    const auto preferences = FeaturePreferences::load(settingsStore_);
    const auto names = [](const QStringList& ids) {
        QStringList values;
        for (const auto& id : ids) if (auto feature = FeatureRegistry::fromId(id)) values << FeatureRegistry::info(*feature).name;
        return values.isEmpty() ? QString("none") : values.join(", ");
    };
    const auto config = Settings::load(settingsStore_);
    const auto& metrics = controller_.metrics();
    productText_->setPlainText(QString("Enabled features: %1\nQuick actions: %2\nProfile enabled: %3; serialized characters: %4\n"
        "Provider: %5; model configured: %6; credential status: %7\nLast request provider: %8; duration: %9 ms; outcome: %10")
        .arg(names(preferences.enabled),names(preferences.quickActions),settingsStore_.value("profile/enabled",false).toBool()?"yes":"no")
        .arg(Profile::load(settingsStore_).serialize().size())
        .arg(config.providerId,config.model.isEmpty()?"no":"yes",secrets_.credentialStatus(config.providerId),metrics.provider)
        .arg(metrics.durationMs).arg(metrics.outcome));
}
}
