#include "SelectionResolver.h"
namespace ws {
QString selectionSourceName(SelectionSource source) {
    switch(source) {
    case SelectionSource::AtSpi: return "AT-SPI";
    case SelectionSource::PrimarySelection: return "X11 PRIMARY";
    case SelectionSource::Clipboard: return "Clipboard";
    }
    return "Unknown";
}
SelectionResolver::SelectionResolver(Read a, Read p, Read c)
    : atspi_(std::move(a)), primary_(std::move(p)), clipboard_(std::move(c)) {}
std::optional<ResolvedSelection> SelectionResolver::resolve() {
    const auto valid=[](const std::optional<Selection>& s) {
        return s && s->text.size()<=100000 && !s->text.trimmed().isEmpty();
    };
    for(const auto& [read,source] : {std::pair{&atspi_,SelectionSource::AtSpi},
            std::pair{&primary_,SelectionSource::PrimarySelection},std::pair{&clipboard_,SelectionSource::Clipboard}}) {
        if(!*read) continue;
        const auto value=(*read)();
        if(valid(value)) return ResolvedSelection{*value,source};
    }
    return {};
}
}
