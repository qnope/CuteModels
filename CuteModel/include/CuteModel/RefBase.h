#pragma once

#include <QObject>
#include <QPersistentModelIndex>

namespace cute {

class RefBase : public QObject
{
    Q_OBJECT
public:
    const QPersistentModelIndex &index() const { return m_index; }

signals:
    void valueChanged();
    void underlyingHierarchyChanged();

protected:
    explicit RefBase(QPersistentModelIndex index);

    QPersistentModelIndex m_index;
};

bool isInside(const QModelIndex &index,
              const QModelIndex &topLeft,
              const QModelIndex &bottomRight);

bool isInParentChain(const QModelIndex &index, const QModelIndex &ancestor);

}
