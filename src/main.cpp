#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QTimer>
#include "ai/OpenAIProvider.h"
#include "platform/PlatformFactory.h"
#include "platform/ClipboardSelectionProvider.h"
#include "app/Application.h"
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("WorkSidekick"); app.setApplicationName("WorkSidekick"); app.setApplicationVersion("0.1.0");
    QCommandLineParser parser; parser.setApplicationDescription("Explicit clipboard AI assistant");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"smoke-test", "Open windows with synthetic data, make no network calls, then exit."});
    parser.process(app);
    QSettings settings;
    auto secrets = ws::createSecretStore();
    ws::ClipboardSelectionProvider clipboard(*app.clipboard());
    ws::OpenAIProvider ai;
    ws::AppController controller(ai, *secrets, clipboard, settings);
    ws::Application application(controller, *secrets, settings);
    application.start();
    if (parser.isSet("smoke-test")) {
        QTimer::singleShot(0, &app, [&] {
            const bool ok = application.smokeCheck();
            QTimer::singleShot(200, &app, [&, ok] { app.exit(ok ? 0 : 1); });
        });
    }
    return app.exec();
}
