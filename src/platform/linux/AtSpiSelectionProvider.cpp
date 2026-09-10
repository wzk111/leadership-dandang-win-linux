#include "AtSpiSelectionProvider.h"
#include <QElapsedTimer>
namespace ws {
SelectionRead AtSpiSelectionProvider::read(IAtSpiTextSource& source) {
    const int count = source.selectionCount();
    if (count < 0) return {{}, "Text interface unavailable or accessibility error"};
    if (count == 0) return {{}, "Selection count = 0"};
    QElapsedTimer budget; budget.start();
    for (int i = 0; i < qMin(count, 32); ++i) {
        if (budget.elapsed() > 2000) return {{}, "Accessibility query budget exceeded"};
        const auto range = source.range(i);
        if (!range) return {{}, "Selection range retrieval error"};
        const auto [start, end] = *range;
        if (start < 0 || end <= start) continue;
        if (qint64(end) - start > 100000) return {{}, "Selection too large"};
        const auto text = source.text(start, end);
        if (!text) return {{}, "Text retrieval error"};
        if (text->size() > 100000) return {{}, "Selection too large"};
        if (text->trimmed().isEmpty()) continue;
        auto rect = source.rectangle(start, end);
        if (rect && (rect->width() <= 0 || rect->height() <= 0 ||
            qAbs(qint64(rect->x())) > 1000000 || qAbs(qint64(rect->y())) > 1000000 ||
            rect->width() > 1000000 || rect->height() > 1000000)) rect.reset();
        auto app = source.application().left(256);
        if (app.isEmpty()) app = "Unknown application";
        return {Selection{*text, rect, app}, rect ? "Text retrieved; screen rectangle available" : "Text retrieved; geometry unavailable"};
    }
    return {{}, count > 32 ? "Selection count exceeds query limit" : "No non-empty selection"};
}
}
