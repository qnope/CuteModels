#pragma once

#include <QAbstractItemModel>
#include <QPersistentModelIndex>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace cute {

template <typename T>
class ItemProxy
{
public:
    ItemProxy(T &value,
              QPersistentModelIndex workingIndex,
              QPersistentModelIndex left,
              QPersistentModelIndex right)
        : m_value(value)
        , m_index(std::move(workingIndex))
        , m_left(std::move(left))
        , m_right(std::move(right))
    {
        if (!m_index.isValid() || !m_left.isValid() || !m_right.isValid())
            throw std::invalid_argument(
                "ItemProxy: requires valid model indices");
    }

    ItemProxy(const ItemProxy &) = delete;
    ItemProxy &operator=(const ItemProxy &) = delete;
    ItemProxy(ItemProxy &&) = delete;

    ~ItemProxy()
    {
        if constexpr (!std::is_const_v<T>) {
            if (!m_left.isValid())
                return;
            auto *model = const_cast<QAbstractItemModel *>(m_left.model());
            emit model->dataChanged(m_left, m_right);
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
    QPersistentModelIndex m_left;
    QPersistentModelIndex m_right;
};

}
