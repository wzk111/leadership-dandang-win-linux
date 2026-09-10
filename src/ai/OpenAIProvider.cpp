#include "OpenAIProvider.h"
namespace ws {
OpenAIProvider::OpenAIProvider(QObject* parent, QUrl endpoint, int timeoutMs)
    : IAIProvider(parent), endpoint_(endpoint), timeoutMs_(timeoutMs) {}
bool OpenAIProvider::generate(const AIRequest&) { return false; }
void OpenAIProvider::cancel() {}
bool OpenAIProvider::busy() const { return false; }
AIResult OpenAIProvider::decode(int, QNetworkReply::NetworkError, const QByteArray&) { return {}; }
}
