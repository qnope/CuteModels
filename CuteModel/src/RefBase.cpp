#include "CuteModel/RefBase.h"

#include <QAbstractItemModel>

namespace cute {

RefBase::RefBase(QPersistentModelIndex index)
    : QObject()
    , m_index(std::move(index))
{
    if (const QAbstractItemModel *model = m_index.model()) {
        connect(model, &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                    if (isInside(m_index, topLeft, bottomRight))
                        emit valueChanged();
                    else if (isInParentChain(topLeft, m_index))
                        emit underlyingHierarchyChanged();
                });

        // rowsRemoved, columnsRemoved and layoutChanged can invalidate the
        // persistent index this Ref tracks; modelReset invalidates every
        // persistent index. After any of them, if the index is no longer
        // valid the underlying value is gone for good.
        connect(model, &QAbstractItemModel::rowsRemoved, this,
                [this] { notifyIfDestroyed(); });
        connect(model, &QAbstractItemModel::columnsRemoved, this,
                [this] { notifyIfDestroyed(); });
        connect(model, &QAbstractItemModel::layoutChanged, this,
                [this] { notifyIfDestroyed(); });
        connect(model, &QAbstractItemModel::modelReset, this,
                [this] { notifyIfDestroyed(); });
    }
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
