#pragma once

#include <QAbstractItemModel>
#include <QPersistentModelIndex>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace cute {

// Proxy over one cell of a model. Holds a reference to the underlying T (so
// reads and writes pass straight through to storage — no QVariant round-trip)
// and a QPersistentModelIndex (used only by the destructor to address the
// dataChanged signal).
//
// Const-ness is selected by the template parameter:
//   ItemProxy<const T>   — read-only view; destructor is a no-op.
//   ItemProxy<T>         — mutable view; destructor emits dataChanged(idx, idx)
//                          exactly once, so multiple in-lifetime mutations
//                          batch into a single notification.
//
// Non-copyable / non-movable to keep the "one proxy -> at most one
// dataChanged" RAII invariant. Returning by value from BaseModel::at /
// getMutable works thanks to C++17 mandatory copy elision (the prvalue
// initializes the named variable directly).
//
// Invalid index at construction terminates — callers are expected to have
// resolved the index first. If the row is removed during the proxy's
// lifetime, the QPersistentModelIndex becomes invalid and the destructor
// silently skips the dataChanged emit. (The held T& itself is invalidated
// by the same erase; callers must stop using the proxy after such an op.)
template <typename T>
class ItemProxy
{
public:
    explicit ItemProxy(T &value, QPersistentModelIndex index)
        : m_value(value)
        , m_index(std::move(index))
    {
        if (!m_index.isValid())
            std::terminate();
    }

    ItemProxy(const ItemProxy &) = delete;
    ItemProxy &operator=(const ItemProxy &) = delete;
    ItemProxy(ItemProxy &&) = delete;

    ~ItemProxy()
    {
        if constexpr (!std::is_const_v<T>) {
            if (!m_index.isValid())
                return;
            auto *model = const_cast<QAbstractItemModel *>(m_index.model());
            emit model->dataChanged(QModelIndex(m_index), QModelIndex(m_index));
        }
    }

    // Pointer-like accessors. The returned T is exactly the template T —
    // const-correctness flows from the instantiation.
    T &operator*() const noexcept { return m_value; }
    T *operator->() const noexcept { return std::addressof(m_value); }

    // Convenience assignment. Calling this on an ItemProxy<const T> fires
    // the static_assert with a clear, dedicated diagnostic instead of the
    // assignment-to-const-reference error that would come from m_value's
    // type otherwise.
    ItemProxy &operator=(T value)
    {
        static_assert(!std::is_const_v<T>,
                      "ItemProxy<const T>::operator= is not allowed — "
                      "the proxy is read-only");
        m_value = std::move(value);
        return *this;
    }

    const QPersistentModelIndex &index() const noexcept { return m_index; }

private:
    T &m_value;
    QPersistentModelIndex m_index;
};

} // namespace cute
