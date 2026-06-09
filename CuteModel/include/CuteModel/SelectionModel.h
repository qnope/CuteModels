#pragma once

#include "CuteModel/AbstractSourceModel.h"
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

enum class SelectionMode
{
    List,
    Table
};

template <typename T>
class SelectionModel : public QItemSelectionModel
{
public:
    using Ref = typename AbstractSourceModel<T>::Ref;

    using QItemSelectionModel::isSelected;
    using QItemSelectionModel::select;

    explicit SelectionModel(AbstractSourceModel<T> *model,
                            SelectionMode mode = SelectionMode::List,
                            QObject *parent = nullptr)
        : QItemSelectionModel(model, parent)
        , m_model(model)
        , m_mode(mode)
    {}

    AbstractSourceModel<T> *sourceModel() const { return m_model; }
    SelectionMode mode() const { return m_mode; }

    std::vector<T> selectedValues() const
    {
        static_assert(std::is_copy_constructible_v<T>,
                      "selectedValues requires a copy-constructible T");
        std::vector<T> values;
        for (const QModelIndex &index : selectedTypedIndexes())
            values.push_back(m_model->getStorageValue(index));
        return values;
    }

    std::optional<T> currentValue() const
    {
        static_assert(std::is_copy_constructible_v<T>,
                      "currentValue requires a copy-constructible T");
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return std::nullopt;
        return m_model->getStorageValue(index);
    }

    template <typename R = Ref>
    std::vector<std::unique_ptr<R>> selectedRefs() const
    {
        std::vector<std::unique_ptr<R>> refs;
        for (const QModelIndex &index : selectedTypedIndexes())
            refs.push_back(m_model->template getRef<R>(index));
        return refs;
    }

    template <typename R = Ref>
    std::unique_ptr<R> currentRef() const
    {
        return m_model->template getRef<R>(currentIndex());
    }

    void select(const T &value, QItemSelectionModel::SelectionFlags command)
    {
        static_assert(is_equality_comparable_v<T>,
                      "select(const T&) requires an equality-comparable T");
        QItemSelection selection;
        for (const QModelIndex &index : indexesForValue(value))
            selection.select(index, index);
        QItemSelectionModel::select(selection, command);
    }

    void deselect(const T &value)
    {
        static_assert(is_equality_comparable_v<T>,
                      "deselect(const T&) requires an equality-comparable T");
        select(value, QItemSelectionModel::Deselect);
    }

    bool isSelected(const T &value) const
    {
        static_assert(is_equality_comparable_v<T>,
                      "isSelected(const T&) requires an equality-comparable T");
        for (const QModelIndex &index : indexesForValue(value))
            if (QItemSelectionModel::isSelected(index))
                return true;
        return false;
    }

private:
    std::vector<QModelIndex> selectedTypedIndexes() const
    {
        const QModelIndexList selected = selectedIndexes();
        if (m_mode == SelectionMode::Table)
            return std::vector<QModelIndex>(selected.cbegin(), selected.cend());

        std::vector<QModelIndex> rows;
        for (const QModelIndex &index : selected) {
            const QModelIndex column0 = index.sibling(index.row(), 0);
            if (std::find(rows.cbegin(), rows.cend(), column0) == rows.cend())
                rows.push_back(column0);
        }
        return rows;
    }

    std::vector<QModelIndex> indexesForValue(const T &value) const
    {
        const ColumnPolicy columns = (m_mode == SelectionMode::List)
                                         ? ColumnPolicy::FirstColumnOnly
                                         : ColumnPolicy::AllColumns;
        return indexesMatching<T>(
            m_model, [&value](const T &candidate) { return candidate == value; },
            columns);
    }

    AbstractSourceModel<T> *m_model;
    SelectionMode m_mode;
};

}
