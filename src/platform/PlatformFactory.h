#pragma once
#include <memory>
#include "ISecretStore.h"
namespace ws {
std::unique_ptr<ISecretStore> createSecretStore();
}
