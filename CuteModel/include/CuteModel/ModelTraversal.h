#pragma once

#include "CuteModel/BaseModel.h"

#include <QModelIndex>

#include <utility>
#include <vector>

namespace cute {

// Visite récursivement tous les index valides du modèle, dans l'ordre
// (ligne, colonne), puis descend dans les enfants de chaque ligne via son index
// de colonne 0. `visit` est un invocable `void(const T &, const QModelIndex &)`.
template <typename T, typename Visitor>
void forEachIndex(const BaseModel<T> &model, Visitor &&visit,
                  const QModelIndex &parent = QModelIndex())
{
    const int rows = model.rowCount(parent);
    const int columns = model.columnCount(parent);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QModelIndex idx = model.index(row, column, parent);
            if (!idx.isValid())
                continue;
            visit(model.getStorageValue(idx), idx);
        }

        const QModelIndex childParent = model.index(row, 0, parent);
        if (childParent.isValid() && model.hasChildren(childParent))
            forEachIndex(model, visit, childParent);
    }
}

// Construit le vecteur des index dont la valeur T satisfait le prédicat.
// `pred` est un invocable `bool(const T &)`. Bâti au-dessus de forEachIndex.
template <typename T, typename Predicate>
std::vector<QModelIndex> indexesMatching(const BaseModel<T> &model, Predicate &&pred,
                                         const QModelIndex &parent = QModelIndex())
{
    std::vector<QModelIndex> result;
    forEachIndex<T>(
        model,
        [&](const T &value, const QModelIndex &index) {
            if (pred(value))
                result.push_back(index);
        },
        parent);
    return result;
}

}
