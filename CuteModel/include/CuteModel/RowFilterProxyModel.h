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
        setSourceModel(source); // remontée implicite AbstractSourceModel<T>* -> QAbstractItemModel*
    }

    // Une seule surcharge. Un AbstractSourceModel<T>* passé par l'appelant est
    // converti implicitement par la remontée de pointeur dérivé->base (pas de
    // static_cast, pas de surcharge dédiée qui récurserait). Une source non
    // nulle qui n'est pas un AbstractSourceModel<T> est une erreur de
    // programmation -> exception.
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
        m_accessor = m_source; // AbstractSourceModel<T> EST-UN ValueModelAccessor<T>
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

    // ValueModelAccessor<T> : on raisonne sur des index PROXY, on délègue à la source.
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
