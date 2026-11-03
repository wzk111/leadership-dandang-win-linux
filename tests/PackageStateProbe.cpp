// Test-only delivery probe. This executable must never enter a product package.
#include <QCoreApplication>
#include <QEventLoop>
#include <QSettings>
#include <QTimer>
#include <cstdio>
#include <cstdlib>
#include "core/Core.h"
#include "core/FeaturePreferences.h"
#include "core/Profile.h"
#include "platform/PlatformFactory.h"

namespace {
const QStringList providers{"openai", "anthropic", "gemini", "openai_compatible"};

[[noreturn]] void fail() {
    // Never print a provider response, settings content, profile or credential.
    std::fputs("Package state probe failed.\n", stderr);
    std::fflush(stderr);
    // A native vault backend may still have a blocked worker. Do not let its
    // global thread-pool shutdown turn a bounded probe into an unbounded hang.
    std::_Exit(1);
}

QString sentinel(const QString& provider) {
    return "WorkSidekick M6 synthetic sentinel for WorkSidekick/" + provider + "; not an API key";
}

ws::SecretResult read(ws::ISecretStore& store, const QString& provider) {
    if (!store.selectProvider(provider)) fail();
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    ws::SecretResult result;
    bool completed = false;
    const auto finished = QObject::connect(&store, &ws::ISecretStore::readFinished, &loop,
        [&](const ws::SecretResult& value) { result = value; completed = true; loop.quit(); });
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    deadline.start(5000);
    if (!store.read()) fail();
    if (!completed) loop.exec();
    QObject::disconnect(finished);
    if (!completed || !result.error.isEmpty()) fail();
    return result;
}

void write(ws::ISecretStore& store, const QString& provider, bool remove = false) {
    if (!store.selectProvider(provider)) fail();
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    bool completed = false, success = false;
    const auto finished = QObject::connect(&store, &ws::ISecretStore::writeFinished, &loop,
        [&](bool ok, const QString&) { success = ok; completed = true; loop.quit(); });
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    deadline.start(5000);
    if (!(remove ? store.remove() : store.save(sentinel(provider)))) fail();
    if (!completed) loop.exec();
    QObject::disconnect(finished);
    if (!completed || !success) fail();
}

bool stateMatches(QSettings& settings) {
    // Settings::load can migrate this legacy value. Verification is read-only.
    if (settings.contains("ai/model")) fail();
    const auto ai = ws::Settings::load(settings);
    const auto profile = ws::Profile::load(settings);
    const auto features = ws::FeaturePreferences::load(settings);
    return ai.providerId == "anthropic" && ai.model == "synthetic-m6-model"
        && profile.role == "Synthetic QA role" && settings.value("profile/enabled").toBool()
        && features.enabled == QStringList{"summarize"} && features.quickActions == QStringList{"summarize"}
        && settings.contains("ui/automaticPopup") && !settings.value("ui/automaticPopup").toBool()
        && settings.contains("shortcut/enabled") && !settings.value("shortcut/enabled").toBool()
        && settings.status() == QSettings::NoError;
}

void verify(ws::ISecretStore& store, QSettings& settings) {
    if (!stateMatches(settings)) fail();
    for (const auto& provider : providers) {
        if (read(store, provider).key != sentinel(provider)) fail();
    }
}

void seed(ws::ISecretStore& store, QSettings& settings) {
#ifdef Q_OS_LINUX
    if (qEnvironmentVariable("WORKSIDEKICK_TEST_ISOLATED_KEYRING") != "1") fail();
    // The opt-in alone is not proof of an empty disposable account. Refuse all
    // pre-existing settings before any model load (which can migrate) or save.
    if (!settings.allKeys().isEmpty() || settings.status() != QSettings::NoError) fail();
    // Check every account before the first write. Never overwrite an unexpected
    // key, even if an isolated-keyring opt-in was set accidentally.
    for (const auto& provider : providers) {
        const auto existing = read(store, provider).key;
        if (!existing.isEmpty() && existing != sentinel(provider)) fail();
    }
    ws::Settings ai;
    ai.providerId = "anthropic";
    ai.model = "synthetic-m6-model";
    ws::Profile profile;
    profile.role = "Synthetic QA role";
    const ws::FeaturePreferences features{{"summarize"}, {"summarize"}};
    if (!ai.save(settings) || !profile.save(settings) || !features.save(settings)) fail();
    settings.setValue("profile/enabled", true);
    settings.setValue("ui/automaticPopup", false);
    settings.setValue("shortcut/enabled", false);
    settings.sync();
    if (settings.status() != QSettings::NoError) fail();
    for (const auto& provider : providers) write(store, provider);
    verify(store, settings);
    write(store, "anthropic", true);
    for (const auto& provider : providers) {
        const auto value = read(store, provider).key;
        if (provider == "anthropic" ? !value.isEmpty() : value != sentinel(provider)) fail();
    }
    write(store, "anthropic");
    verify(store, settings);
#else
    Q_UNUSED(store);
    Q_UNUSED(settings);
    fail();
#endif
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("WorkSidekick");
    app.setApplicationName("WorkSidekick");
    if (qEnvironmentVariable("WS_M6_PACKAGE_PROBE") != "1") fail();
    const auto arguments = app.arguments();
    if (arguments.size() != 2 || (arguments[1] != "--verify" && arguments[1] != "--seed")) fail();
    QSettings settings;
    auto store = ws::createSecretStore();
    if (!store) fail();
    if (arguments[1] == "--seed") seed(*store, settings);
    else verify(*store, settings);
    std::puts("Package state probe PASS: production settings, Profile, preferences and 4 distinct provider adapter reads verified.");
    return 0;
}
