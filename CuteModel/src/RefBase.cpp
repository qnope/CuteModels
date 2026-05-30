#include "CuteModel/RefBase.h"

#include <QAbstractItemModel>

namespace cute {

RefBase::RefBase(QPersistentModelIndex index)
    : QObject(const_cast<QAbstractItemModel *>(index.model()))
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
    }
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

} // namespace cute
