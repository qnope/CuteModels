#include "CuteModel/RefBase.h"

#include <QAbstractItemModel>

namespace cute {

RefBase::RefBase(QPersistentModelIndex index)
    : QObject()
    , m_index(std::move(index))
{
    if (const QAbstractItemModel *model = m_index.model()) {
        connect(model, &QAbstractItemModel::dataChanged, this, &RefBase::onDataChanged);

        connect(model, &QAbstractItemModel::rowsRemoved, this, &RefBase::notifyIfDestroyed);
        connect(model, &QAbstractItemModel::columnsRemoved, this, &RefBase::notifyIfDestroyed);
        connect(model, &QAbstractItemModel::layoutChanged, this, &RefBase::notifyIfDestroyed);
        connect(model, &QAbstractItemModel::modelReset, this, &RefBase::notifyIfDestroyed);
    }
}

void RefBase::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (isInside(m_index, topLeft, bottomRight))
        emit valueChanged();
    else if (isInParentChain(topLeft, m_index))
        emit underlyingHierarchyChanged();
}

void RefBase::notifyIfDestroyed()
{
    if (m_destroyed || m_index.isValid())
        return;

    m_destroyed = true;
    emit underlyingValueDestroyed();
}

bool isInside(const QModelIndex &index,
              const QModelIndex &topLeft,
              const QModelIndex &bottomRight)
{
    if (!index.isValid() || !topLeft.isValid() || !bottomRight.isValid())
        return false;

    if (index.model() != topLeft.model())
        return false;

    if (index.parent() != topLeft.parent())
        return false;

    return index.row() >= topLeft.row() && index.row() <= bottomRight.row()
        && index.column() >= topLeft.column() && index.column() <= bottomRight.column();
}

bool isInParentChain(const QModelIndex &index, const QModelIndex &ancestor)
{
    if (!index.isValid() || !ancestor.isValid())
        return false;

    QModelIndex current = index.parent();
    while (current.isValid()) {
        if (current == ancestor)
            return true;
        current = current.parent();
    }
    return false;
}

}
