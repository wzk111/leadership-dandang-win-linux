#include "LinuxSecretStore.h"
#include <QtConcurrent/QtConcurrentRun>
// Qt defines 'signals'; GLib uses that identifier inside its headers.
#pragma push_macro("signals")
#undef signals
#include <libsecret/secret.h>
#pragma pop_macro("signals")
#include <memory>
namespace ws {
namespace {
const SecretSchema schema = {
    "io.worksidekick.ApiKey", SECRET_SCHEMA_NONE,
    {{"account", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}},
    0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
SecretResult operate(int operation, const QString& account, const QString& key) {
    GError* rawError = nullptr;
    const auto accountBytes = account.toUtf8();
    QString value;
    bool ok = true;
    if (operation == 0) {
        std::unique_ptr<gchar, decltype(&secret_password_free)> password(
            secret_password_lookup_sync(&schema, nullptr, &rawError,
                                        "account", accountBytes.constData(), nullptr), secret_password_free);
        if (password) value = QString::fromUtf8(password.get());
    } else if (operation == 1) {
        auto bytes = key.toUtf8();
        ok = secret_password_store_sync(&schema, SECRET_COLLECTION_DEFAULT, "WorkSidekick OpenAI API key",
                                        bytes.constData(), nullptr, &rawError,
                                        "account", accountBytes.constData(), nullptr);
        bytes.fill('\0');
    } else {
        secret_password_clear_sync(&schema, nullptr, &rawError, "account", accountBytes.constData(), nullptr);
    }
    const bool failed = rawError != nullptr || !ok;
    if (rawError) g_error_free(rawError);
    // Do not display raw keyring errors, which may contain sensitive attributes.
    return {value, failed ? "Secret store unavailable or locked. Unlock your login keyring and retry." : QString{}};
}
}
LinuxSecretStore::LinuxSecretStore(QObject* parent, QString account)
    : ISecretStore(parent), account_(std::move(account)) {}
bool LinuxSecretStore::busy() const { return active_; }
QString LinuxSecretStore::description() const { return "libsecret / Secret Service (availability checked on use)"; }
bool LinuxSecretStore::read() { return start(0); }
bool LinuxSecretStore::save(const QString& key) {
    if (key.trimmed().isEmpty()) return false;
    return start(1, key.trimmed());
}
bool LinuxSecretStore::remove() { return start(2); }
bool LinuxSecretStore::start(int operation, const QString& key) {
    if (busy()) return false;
    active_ = true;
    disconnect(&watcher_, nullptr, this, nullptr);
    connect(&watcher_, &QFutureWatcher<SecretResult>::finished, this, [this, operation] {
        active_ = false;
        const auto result = watcher_.result();
        if (operation == 0) emit readFinished(result);
        else emit writeFinished(result.error.isEmpty(), result.error.isEmpty() ? "Secure key storage updated." : result.error);
    });
    // Blocking D-Bus/keyring calls run outside the GUI thread.
    watcher_.setFuture(QtConcurrent::run([operation, account = account_, key] { return operate(operation, account, key); }));
    return true;
}
}
