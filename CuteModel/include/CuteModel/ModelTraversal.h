#pragma once

#include "CuteModel/ValueModelAccessor.h"

#include <QAbstractItemModel>
#include <QModelIndex>

#include <utility>
#include <vector>

namespace cute {

enum class ColumnPolicy
{
    FirstColumnOnly,
    AllColumns
};

namespace detail {

template <typename T, typename Visitor>
void forEachIndexImpl(const QAbstractItemModel &model,
                      const ValueModelAccessor<T> &accessor, Visitor &visit,
                      ColumnPolicy columns, const QModelIndex &parent)
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
            visit(accessor.getStorageValue(idx), idx);
        }

        const QModelIndex childParent = model.index(row, 0, parent);
        if (childParent.isValid() && model.hasChildren(childParent))
            forEachIndexImpl(model, accessor, visit, columns, childParent);
    }
}

}

template <typename T, typename Visitor>
void forEachIndex(const ValueModelAccessor<T> &accessor, Visitor &&visit,
                  ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex())
{
    const auto *model = dynamic_cast<const QAbstractItemModel *>(&accessor);
    if (!model)
        return;
    detail::forEachIndexImpl<T>(*model, accessor, visit, columns, parent);
}

template <typename T, typename Predicate>
std::vector<QModelIndex> indexesMatching(const ValueModelAccessor<T> &accessor,
                                         Predicate &&pred,
                                         ColumnPolicy columns = ColumnPolicy::AllColumns,
                                         const QModelIndex &parent = QModelIndex())
{
    std::vector<QModelIndex> result;
    forEachIndex<T>(
        accessor,
        [&](const T &value, const QModelIndex &index) {
            if (pred(value))
                result.push_back(index);
        },
        columns, parent);
    return result;
}

}
