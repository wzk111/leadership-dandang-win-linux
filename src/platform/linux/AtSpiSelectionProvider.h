#pragma once
#include "../ISelectionProvider.h"
namespace ws {
struct SelectionRead {
    std::optional<Selection> selection;
    QString description;
};
// Small mockable view of one event source, never a desktop-tree traversal.
class IAtSpiTextSource {
public:
    virtual ~IAtSpiTextSource() = default;
    virtual int selectionCount() = 0; // -1 on error or unsupported Text
    virtual std::optional<QPair<int,int>> range(int index) = 0;
    virtual std::optional<QString> text(int start, int end) = 0;
    virtual std::optional<QRect> rectangle(int start, int end) = 0;
    virtual QString application() = 0;
};
class AtSpiSelectionProvider {
public:
    static SelectionRead read(IAtSpiTextSource& source);
};
}
Q_DECLARE_METATYPE(ws::SelectionRead)
