#pragma once

#include "CuteModel/AbstractSourceModel.h"

#include <QModelIndex>

#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

enum class ColumnPolicy
{
    FirstColumnOnly,
    AllColumns
};

// A "cute model" is any type exposing both the QAbstractItemModel navigation surface
// (rowCount/columnCount/index/hasChildren) and the ValueModelAccessor value surface
// (value_type + getStorageValue). Detecting members rather than a concrete base keeps
// traversal open to AbstractSourceModel<T> today and a future Proxy<T> tomorrow.
template <typename M, typename = void>
struct is_cute_model : std::false_type
{
};

template <typename M>
struct is_cute_model<
    M, std::void_t<typename M::value_type,
                   decltype(std::declval<const M &>().rowCount(std::declval<QModelIndex>())),
                   decltype(std::declval<const M &>().columnCount(std::declval<QModelIndex>())),
                   decltype(std::declval<const M &>().index(0, 0, std::declval<QModelIndex>())),
                   decltype(std::declval<const M &>().hasChildren(std::declval<QModelIndex>())),
                   decltype(std::declval<const M &>().getStorageValue(std::declval<QModelIndex>()))>>
    : std::true_type
{
};

template <typename M>
constexpr bool is_cute_model_v = is_cute_model<M>::value;

template <typename Model, typename Visitor,
          typename = std::enable_if_t<is_cute_model_v<Model>>>
void forEachIndex(const Model *model, Visitor &&visit,
                  ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex())
{
    if (!model)
        return;

    const int rows = model->rowCount(parent);
    const int columnCount = model->columnCount(parent);
    const int lastColumn = (columns == ColumnPolicy::FirstColumnOnly)
                               ? (columnCount > 0 ? 1 : 0)
                               : columnCount;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < lastColumn; ++column) {
            const QModelIndex idx = model->index(row, column, parent);
            if (!idx.isValid())
                continue;
            visit(model->getStorageValue(idx), idx);
        }

        const QModelIndex childParent = model->index(row, 0, parent);
        if (childParent.isValid() && model->hasChildren(childParent))
            forEachIndex(model, visit, columns, childParent);
    }
}

template <typename Model, typename Predicate,
          typename = std::enable_if_t<is_cute_model_v<Model>>>
std::vector<QModelIndex> indexesMatching(const Model *model, Predicate &&pred,
                                         ColumnPolicy columns = ColumnPolicy::AllColumns,
                                         const QModelIndex &parent = QModelIndex())
{
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
