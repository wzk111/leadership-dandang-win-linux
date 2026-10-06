#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QTimer>
#include <QTextStream>
#include "ai/OpenAIProvider.h"
#include "platform/PlatformFactory.h"
#include "platform/ClipboardSelectionProvider.h"
#include "app/Application.h"
#include "app/InstanceCoordinator.h"
int main(int argc, char** argv) {
    ws::preparePlatformEventLoop();
    QApplication app(argc, argv);
    app.setOrganizationName("WorkSidekick"); app.setApplicationName("WorkSidekick"); app.setApplicationVersion("0.4.0");
    QCommandLineParser parser; parser.setApplicationDescription("Explicit selection AI assistant");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"smoke-test", "Open windows with synthetic data, make no network calls, then exit."});
    parser.addOption({"trigger","Open local selection actions in the existing instance."});
    parser.process(app);
    ws::InstanceCoordinator instance;
    if(!parser.isSet("smoke-test")) {
        const auto role=instance.start(parser.isSet("trigger"));
        if(role==ws::InstanceCoordinator::Result::Forwarded) return 0;
        if(role==ws::InstanceCoordinator::Result::Failed) {QTextStream(stderr)<<instance.error()<<Qt::endl; return 2;}
    }
    QSettings settings;
    auto secrets = ws::createSecretStore();
    ws::ClipboardSelectionProvider clipboard(*app.clipboard());
    ws::OpenAIProvider ai;
    ws::AppController controller(ai, *secrets, clipboard, settings);
    auto monitor = ws::createSelectionMonitor();
    auto windowPolicy = ws::createWindowPolicy();
    auto resolver=ws::createExplicitResolver(monitor.get(),*app.clipboard());
    auto shortcut=ws::createGlobalShortcut();
    ws::Application application(controller,*secrets,settings,monitor.get(),windowPolicy.get(),resolver.get(),shortcut.get());
    QObject::connect(&instance,&ws::InstanceCoordinator::triggerRequested,&application,&ws::Application::triggerManualActions);
    QObject::connect(&instance,&ws::InstanceCoordinator::openRequested,&application,&ws::Application::openWorkspace);
    application.start(!parser.isSet("trigger"));
    if(parser.isSet("trigger")) QTimer::singleShot(0,&application,&ws::Application::triggerManualActions);
    if (parser.isSet("smoke-test")) {
        QTimer::singleShot(0, &app, [&] {
            const bool ok = application.smokeCheck();
            QTimer::singleShot(200, &app, [&, ok] { app.exit(ok ? 0 : 1); });
        });
    }
    return app.exec();
}
