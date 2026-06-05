#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ModelTraversal.h"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QModelIndexList>

#include <algorithm>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

// Vrai si T expose `a == b` pour deux const T&. Guard pour la sélection par valeur.
template <typename T, typename = void>
struct is_equality_comparable : std::false_type
{
};

template <typename T>
struct is_equality_comparable<
    T, std::void_t<decltype(std::declval<const T &>() == std::declval<const T &>())>>
    : std::true_type
{
};

template <typename T>
constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

// Grain des accesseurs typés :
//  - List  : un T par ligne touchée par la sélection (index de colonne 0).
//  - Table : un T par cellule (index) sélectionnée.
enum class SelectionMode
{
    List,
    Table
};

template <typename T>
class SelectionModel : public QItemSelectionModel
{
public:
    using Ref = typename BaseModel<T>::Ref;

    // Garde visibles les surcharges select()/isSelected() par index de la base
    // à côté des surcharges par valeur ci-dessous.
    using QItemSelectionModel::isSelected;
    using QItemSelectionModel::select;

    explicit SelectionModel(BaseModel<T> *model,
                            SelectionMode mode = SelectionMode::List,
                            QObject *parent = nullptr)
        : QItemSelectionModel(model, parent)
        , m_model(model)
        , m_mode(mode)
    {}

    BaseModel<T> *sourceModel() const { return m_model; }
    SelectionMode mode() const { return m_mode; }

    // --- Lecture typée des éléments sélectionnés -------------------------------

    template <typename U = T, std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
    std::vector<T> selectedValues() const
    {
        std::vector<T> values;
        for (const QModelIndex &index : selectedTypedIndexes())
            values.push_back(m_model->getStorageValue(index));
        return values;
    }

    template <typename U = T, std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
    std::optional<T> currentValue() const
    {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return std::nullopt;
        return m_model->getStorageValue(index);
    }

    template <typename R = Ref>
    std::vector<std::unique_ptr<R>> selectedRefs()
    {
        std::vector<std::unique_ptr<R>> refs;
        for (const QModelIndex &index : selectedTypedIndexes())
            refs.push_back(m_model->template getRef<R>(index));
        return refs;
    }

    template <typename R = Ref>
    std::unique_ptr<R> currentRef()
    {
        return m_model->template getRef<R>(currentIndex());
    }

    // --- Sélection par valeur --------------------------------------------------

    template <typename U = T, std::enable_if_t<is_equality_comparable_v<U>, int> = 0>
    void select(const T &value, QItemSelectionModel::SelectionFlags command)
    {
        QItemSelection selection;
        for (const QModelIndex &index : indexesForValue(value))
            selection.select(index, index);
        QItemSelectionModel::select(selection, command);
    }

    template <typename U = T, std::enable_if_t<is_equality_comparable_v<U>, int> = 0>
    void deselect(const T &value)
    {
        select(value, QItemSelectionModel::Deselect);
    }

    template <typename U = T, std::enable_if_t<is_equality_comparable_v<U>, int> = 0>
    bool isSelected(const T &value) const
    {
        for (const QModelIndex &index : indexesForValue(value))
            if (QItemSelectionModel::isSelected(index))
                return true;
        return false;
    }

private:
    // Index pertinents pour la lecture, selon le mode.
    std::vector<QModelIndex> selectedTypedIndexes() const
    {
        const QModelIndexList selected = selectedIndexes();
        if (m_mode == SelectionMode::Table)
            return std::vector<QModelIndex>(selected.cbegin(), selected.cend());

        // List : une entrée (colonne 0) par ligne touchée, sans doublon.
        std::vector<QModelIndex> rows;
        for (const QModelIndex &index : selected) {
            const QModelIndex column0 = index.sibling(index.row(), 0);
            if (std::find(rows.cbegin(), rows.cend(), column0) == rows.cend())
                rows.push_back(column0);
        }
        return rows;
    }

    // Tous les index dont la valeur stockée vaut `value`, restreints selon le mode.
    std::vector<QModelIndex> indexesForValue(const T &value) const
    {
        std::vector<QModelIndex> matches = indexesMatching<T>(
            *m_model, [&value](const T &candidate) { return candidate == value; });
        if (m_mode == SelectionMode::Table)
            return matches;

        // List : garder un index par ligne (colonne 0). Comme une ligne projette
        // la même valeur sur toutes ses colonnes, cela donne une entrée par ligne.
        std::vector<QModelIndex> column0;
        for (const QModelIndex &index : matches)
            if (index.column() == 0)
                column0.push_back(index);
        return column0;
    }

    BaseModel<T> *m_model;
    SelectionMode m_mode;
};

}
