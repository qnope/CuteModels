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
    void underlyingValueDestroyed();

protected:
    explicit RefBase(QPersistentModelIndex index);

    QPersistentModelIndex m_index;

private:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void notifyIfDestroyed();

    bool m_destroyed = false;
};

bool isInside(const QModelIndex &index,
              const QModelIndex &topLeft,
              const QModelIndex &bottomRight);

bool isInParentChain(const QModelIndex &index, const QModelIndex &ancestor);

}
