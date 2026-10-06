#include "AIService.h"
namespace ws {
AIService::AIService(QObject* p,QMap<QString,IAIProvider*> providers):IAIProvider(p),providers_(providers){}
bool AIService::generate(const AIRequest&){return false;}
void AIService::cancel(){}
bool AIService::busy()const{return false;}
}
