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

// A "cute model" is any type exposing a value_type and deriving from
// AbstractSourceModel<value_type> -- AbstractSourceModel<T> itself today, and any
// future Proxy<T> that derives from it tomorrow.
template <typename M, typename = void>
struct is_cute_model : std::false_type
{
};

template <typename M>
struct is_cute_model<M, std::void_t<typename M::value_type>>
    : std::is_base_of<AbstractSourceModel<typename M::value_type>, M>
{
};

template <typename M>
constexpr bool is_cute_model_v = is_cute_model<M>::value;

template <typename Model, typename Visitor>
void forEachIndex(const Model *model, Visitor &&visit,
                  ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex())
{
    static_assert(is_cute_model_v<Model>,
                  "forEachIndex requires a cute model: a type exposing value_type and "
                  "deriving from AbstractSourceModel<value_type>");

    if (!model)
        return;

    // Access the value interface through the AbstractSourceModel base, where
    // getStorageValue is public (derived overrides may narrow it to protected).
    const AbstractSourceModel<typename Model::value_type> *src = model;

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
            forEachIndex(model, visit, columns, childParent);
    }
}

template <typename Model, typename Predicate>
std::vector<QModelIndex> indexesMatching(const Model *model, Predicate &&pred,
                                         ColumnPolicy columns = ColumnPolicy::AllColumns,
                                         const QModelIndex &parent = QModelIndex())
{
    static_assert(is_cute_model_v<Model>,
                  "indexesMatching requires a cute model: a type exposing value_type and "
                  "deriving from AbstractSourceModel<value_type>");

    using T = typename Model::value_type;
    std::vector<QModelIndex> result;
    forEachIndex(
        model,
        [&](const T &value, const QModelIndex &index) {
            if (pred(value))
                result.push_back(index);
        },
        columns, parent);
    return result;
}

}
