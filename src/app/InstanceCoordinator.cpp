#include "InstanceCoordinator.h"
namespace ws {
InstanceCoordinator::InstanceCoordinator(QString directory,QObject* parent):QObject(parent),directory_(directory) {}
InstanceCoordinator::Result InstanceCoordinator::start(bool) { return Result::Failed; }
}
