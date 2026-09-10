#pragma once
#include <QObject>
#include "core/Core.h"
namespace ws {
enum class AIError { None, MissingKey, MissingModel, Authentication, RateLimited, Network,
                     Timeout, Cancelled, InvalidJson, EmptyResponse, Server, Refused, Incomplete, TooLarge };
struct AIResult { QString text; AIError error = AIError::None; QString message; };
struct AIRequest { Prompt prompt; QString model; QString apiKey; };
class IAIProvider : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual bool generate(const AIRequest& request) = 0;
    virtual void cancel() = 0;
    virtual bool busy() const = 0;
signals:
    void completed(const ws::AIResult& result);
};
}
Q_DECLARE_METATYPE(ws::AIResult)
