#pragma once

#include <QAbstractItemModel>
#include <QPersistentModelIndex>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace cute {

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
            const QModelIndex parent = m_index.parent();
            const int lastCol = model->columnCount(parent) - 1;
            const QModelIndex left = model->index(m_index.row(), 0, parent);
            const QModelIndex right = model->index(m_index.row(), lastCol, parent);
            emit model->dataChanged(left, right);
        }
    }

    T &operator*() const noexcept { return m_value; }
    T *operator->() const noexcept { return std::addressof(m_value); }

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

}
