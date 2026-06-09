#pragma once

#include "CuteModel/AbstractSourceModel.h"

#include <QModelIndex>

#include <type_traits>
#include <vector>

namespace cute {

enum class ColumnPolicy
{
    FirstColumnOnly,
    AllColumns
};

// A "cute model" for value type T is any model M deriving from AbstractSourceModel<T>
// -- AbstractSourceModel<T> itself today, and any future Proxy<T> that derives from it.
template <typename M, typename T>
struct is_cute_model_T : std::is_base_of<AbstractSourceModel<T>, M>
{
};

template <typename M, typename T>
constexpr bool is_cute_model_T_v = is_cute_model_T<M, T>::value;

template <typename T, typename Model, typename Visitor>
void forEachIndex(const Model *model, Visitor &&visit,
                  ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex())
{
    static_assert(is_cute_model_T_v<Model, T>,
                  "forEachIndex<T> requires a model deriving from AbstractSourceModel<T>");

    if (!model)
        return;

    // Access the value interface through the AbstractSourceModel base, where
    // getStorageValue is public (derived overrides may narrow it to protected).
    const AbstractSourceModel<T> *src = model;

    const int rows = src->rowCount(parent);
    const int columnCount = src->columnCount(parent);
    const int lastColumn = (columns == ColumnPolicy::FirstColumnOnly)
                               ? (columnCount > 0 ? 1 : 0)
                               : columnCount;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < lastColumn; ++column) {
            const QModelIndex idx = src->index(row, column, parent);
            if (!idx.isValid())
                continue;
            visit(src->getStorageValue(idx), idx);
        }

        const QModelIndex childParent = src->index(row, 0, parent);
        if (childParent.isValid() && src->hasChildren(childParent))
            forEachIndex<T>(model, visit, columns, childParent);
    }
}

template <typename T, typename Model, typename Predicate>
std::vector<QModelIndex> indexesMatching(const Model *model, Predicate &&pred,
                                         ColumnPolicy columns = ColumnPolicy::AllColumns,
                                         const QModelIndex &parent = QModelIndex())
{
    static_assert(is_cute_model_T_v<Model, T>,
                  "indexesMatching<T> requires a model deriving from AbstractSourceModel<T>");

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
