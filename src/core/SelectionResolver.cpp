#include "SelectionResolver.h"
namespace ws {
QString selectionSourceName(SelectionSource) { return {}; }
SelectionResolver::SelectionResolver(Read a, Read p, Read c) : atspi_(a), primary_(p), clipboard_(c) {}
std::optional<ResolvedSelection> SelectionResolver::resolve() { return {}; }
}
