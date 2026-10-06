#pragma once
#include "IAIProvider.h"
#include <QMap>
namespace ws {
class AIService : public IAIProvider {
 Q_OBJECT
public:
    explicit AIService(QObject* parent=nullptr,QMap<QString,IAIProvider*> providers={});
    bool generate(const AIRequest&)override;
    void cancel()override;
    bool busy()const override;
private:
    QMap<QString,IAIProvider*> providers_;
    IAIProvider* active_=nullptr;
    bool pendingError_=false;
};
}
