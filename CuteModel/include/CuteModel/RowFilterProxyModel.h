#pragma once

#include "CuteModel/ValueModelAccessor.h"

#include <QModelIndex>
#include <QSortFilterProxyModel>

#include <functional>
#include <stdexcept>
#include <utility>

namespace cute {

template <typename T>
class RowFilterProxyModel : public QSortFilterProxyModel, public ValueModelAccessor<T>
{
public:
    using Ref = typename ValueModelAccessor<T>::Ref;
    using FilterPredicate = std::function<bool(const T &, const QModelIndex &)>;

    explicit RowFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    explicit RowFilterProxyModel(QAbstractItemModel *source, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(source);
    }

    void setSourceModel(QAbstractItemModel *source) override
    {
        if (source) {
            m_accessor = dynamic_cast<ValueModelAccessor<T> *>(source);
            if (!m_accessor)
                throw std::invalid_argument(
                    "RowFilterProxyModel::setSourceModel requires a ValueModelAccessor<T>");
        } else {
            m_accessor = nullptr;
        }
        QSortFilterProxyModel::setSourceModel(source);
    }

    ValueModelAccessor<T> *sourceAccessor() const { return m_accessor; }

    void setFilterPredicate(FilterPredicate predicate)
    {
        m_predicate = std::move(predicate);
        invalidateFilter();
    }

    bool hasFilter() const { return static_cast<bool>(m_predicate); }

    void clearFilter() { setFilterPredicate({}); }

    typename ValueModelAccessor<T>::ResolvedSource
    resolveSource(const QModelIndex &index) override
    {
        if (!m_accessor)
            return {this, index};
        return m_accessor->resolveSource(mapToSource(index));
    }

    const T &getStorageValue(const QModelIndex &index) const override
    {
        return m_accessor->getStorageValue(mapToSource(index));
    }

    void setStorageValue(const QModelIndex &index, T value) override
    {
        m_accessor->setStorageValue(mapToSource(index), std::move(value));
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (!m_predicate || !m_accessor)
            return true;
        const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!sourceIndex.isValid())
            return true;
        return m_predicate(m_accessor->getStorageValue(sourceIndex), sourceIndex);
    }

private:
    ValueModelAccessor<T> *m_accessor = nullptr;
    FilterPredicate m_predicate;
};

}
