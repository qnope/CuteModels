#pragma once

#include "CuteModel/AbstractSourceModel.h"
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
    using Ref = typename AbstractSourceModel<T>::Ref;
    using FilterPredicate = std::function<bool(const T &, const QModelIndex &)>;

    explicit RowFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    explicit RowFilterProxyModel(AbstractSourceModel<T> *source,
                                 QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(source);
    }

    void setSourceModel(QAbstractItemModel *source) override
    {
        if (source) {
            m_source = dynamic_cast<AbstractSourceModel<T> *>(source);
            if (!m_source)
                throw std::invalid_argument(
                    "RowFilterProxyModel::setSourceModel requires an AbstractSourceModel<T>");
        } else {
            m_source = nullptr;
        }
        m_accessor = m_source;
        QSortFilterProxyModel::setSourceModel(source);
    }

    AbstractSourceModel<T> *typedSourceModel() const { return m_source; }

    void setFilterPredicate(FilterPredicate predicate)
    {
        m_predicate = std::move(predicate);
        invalidateFilter();
    }

    bool hasFilter() const { return static_cast<bool>(m_predicate); }

    void clearFilter() { setFilterPredicate({}); }

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
        if (!m_predicate || !m_source)
            return true;
        const QModelIndex sourceIndex = m_source->index(sourceRow, 0, sourceParent);
        if (!sourceIndex.isValid())
            return true;
        return m_predicate(m_accessor->getStorageValue(sourceIndex), sourceIndex);
    }

private:
    AbstractSourceModel<T> *m_source = nullptr;
    ValueModelAccessor<T> *m_accessor = nullptr;
    FilterPredicate m_predicate;
};

}
