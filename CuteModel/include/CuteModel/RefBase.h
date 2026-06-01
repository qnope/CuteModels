#pragma once

#include <QObject>
#include <QPersistentModelIndex>

namespace cute {

// Base, non-templated part of Ref<T>. It follows a single model item through a
// QPersistentModelIndex and re-emits change notifications about that item (or its
// descendants). Being a plain QObject, it can carry the Q_OBJECT signals that the
// templated Ref<T> subclass inherits (moc cannot process templates directly).
class RefBase : public QObject
{
    Q_OBJECT
public:
    const QPersistentModelIndex &index() const { return m_index; }

signals:
    void valueChanged();
    void underlyingHierarchyChanged();

protected:
    // Parent-less QObject: the Ref's lifetime is owned by whoever holds it
    // (BaseModel::getRef hands back a std::unique_ptr), not by the model.
    // Protected so only the Ref<T> subclass can construct the base.
    explicit RefBase(QPersistentModelIndex index);

    QPersistentModelIndex m_index;
};

// True when `index` is covered by the rectangle [topLeft, bottomRight]
// (same parent, row/column inside the range).
bool isInside(const QModelIndex &index,
              const QModelIndex &topLeft,
              const QModelIndex &bottomRight);

// True when `index` is a descendant of `ancestor` (walking up the parent chain).
bool isInParentChain(const QModelIndex &index, const QModelIndex &ancestor);

} // namespace cute
