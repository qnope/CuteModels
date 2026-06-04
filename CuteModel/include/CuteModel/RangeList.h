#pragma once

#include <QAbstractItemModel>
#include <QPersistentModelIndex>

#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

template <typename U>
class RangeList
{
public:
    using value_type = std::remove_const_t<U>;
    using iterator = std::conditional_t<std::is_const_v<U>,
                                        typename std::vector<value_type>::const_iterator,
                                        typename std::vector<value_type>::iterator>;

    RangeList(iterator begin,
              iterator end,
              QPersistentModelIndex topLeft,
              QPersistentModelIndex bottomRight)
        : m_begin(std::move(begin))
        , m_end(std::move(end))
        , m_topLeft(std::move(topLeft))
        , m_bottomRight(std::move(bottomRight))
    {
        if (m_begin != m_end && (!m_topLeft.isValid() || !m_bottomRight.isValid()))
            std::terminate();
    }

    RangeList(const RangeList &) = delete;
    RangeList &operator=(const RangeList &) = delete;
    RangeList(RangeList &&) = delete;
    RangeList &operator=(RangeList &&) = delete;

    ~RangeList()
    {
        if constexpr (!std::is_const_v<U>) {
            if (m_begin == m_end)
                return;
            if (!m_topLeft.isValid())
                return;
            auto *model = const_cast<QAbstractItemModel *>(m_topLeft.model());
            emit model->dataChanged(m_topLeft, m_bottomRight);
        }
    }

    iterator begin() const noexcept { return m_begin; }
    iterator end() const noexcept { return m_end; }
    int size() const noexcept { return static_cast<int>(m_end - m_begin); }
    bool empty() const noexcept { return m_begin == m_end; }

private:
    iterator m_begin;
    iterator m_end;
    QPersistentModelIndex m_topLeft;
    QPersistentModelIndex m_bottomRight;
};

}
