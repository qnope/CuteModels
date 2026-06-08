#pragma once

#include "CuteModel/AbstractSourceModel.h"

#include <QModelIndex>

#include <utility>
#include <vector>

namespace cute {

enum class ColumnPolicy
{
    FirstColumnOnly,
    AllColumns
};

template <typename T, typename Visitor>
void forEachIndex(const AbstractSourceModel<T> &model, Visitor &&visit,
                  ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex())
{
    const int rows = model.rowCount(parent);
    const int columnCount = model.columnCount(parent);
    const int lastColumn = (columns == ColumnPolicy::FirstColumnOnly)
                               ? (columnCount > 0 ? 1 : 0)
                               : columnCount;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < lastColumn; ++column) {
            const QModelIndex idx = model.index(row, column, parent);
            if (!idx.isValid())
                continue;
            visit(model.getStorageValue(idx), idx);
        }

        const QModelIndex childParent = model.index(row, 0, parent);
        if (childParent.isValid() && model.hasChildren(childParent))
            forEachIndex(model, visit, columns, childParent);
    }
}

template <typename T, typename Predicate>
std::vector<QModelIndex> indexesMatching(const AbstractSourceModel<T> &model, Predicate &&pred,
                                         ColumnPolicy columns = ColumnPolicy::AllColumns,
                                         const QModelIndex &parent = QModelIndex())
{
    std::vector<QModelIndex> result;
    forEachIndex<T>(
        model,
        [&](const T &value, const QModelIndex &index) {
            if (pred(value))
                result.push_back(index);
        },
        columns, parent);
    return result;
}

}
