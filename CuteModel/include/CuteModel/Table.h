#pragma once

#include <QtGlobal>

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace cute {

template <typename T>
class Table
{
public:
    Table() = default;

    Table(int rows, int columns)
        : Table(rows, columns, T())
    {}

    Table(int rows, int columns, const T &defaultValue)
        : m_columnCount(columns < 0 ? 0 : columns)
    {
        const int rowsClamped = rows < 0 ? 0 : rows;
        m_rows.assign(static_cast<std::size_t>(rowsClamped),
                      std::vector<T>(static_cast<std::size_t>(m_columnCount),
                                     defaultValue));
    }

    int rows() const noexcept { return static_cast<int>(m_rows.size()); }
    int columns() const noexcept { return m_columnCount; }
    bool empty() const noexcept { return m_rows.empty() || m_columnCount == 0; }

    const T &at(int row, int column) const noexcept
    {
        Q_ASSERT(row >= 0 && row < rows());
        Q_ASSERT(column >= 0 && column < m_columnCount);
        return m_rows[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>(column)];
    }

    T &at(int row, int column) noexcept
    {
        Q_ASSERT(row >= 0 && row < rows());
        Q_ASSERT(column >= 0 && column < m_columnCount);
        return m_rows[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>(column)];
    }

    void insert_rows(int row, int count, const T &value)
    {
        Q_ASSERT(row >= 0 && row <= rows());
        Q_ASSERT(count >= 0);
        if (count == 0)
            return;
        m_rows.insert(m_rows.begin() + row,
                      static_cast<std::size_t>(count),
                      std::vector<T>(static_cast<std::size_t>(m_columnCount),
                                     value));
    }

    void insert_rows(int row, std::vector<std::vector<T>> rowsData)
    {
        Q_ASSERT(row >= 0 && row <= rows());
#ifdef QT_DEBUG
        for (const auto &r : rowsData)
            Q_ASSERT(static_cast<int>(r.size()) == m_columnCount);
#endif
        if (rowsData.empty())
            return;
        m_rows.insert(m_rows.begin() + row,
                      std::make_move_iterator(rowsData.begin()),
                      std::make_move_iterator(rowsData.end()));
    }

    void erase_rows(int first, int last)
    {
        Q_ASSERT(first >= 0 && last >= first && last < rows());
        m_rows.erase(m_rows.begin() + first, m_rows.begin() + last + 1);
    }

    void resize_rows(int count, const T &defaultValue = T())
    {
        Q_ASSERT(count >= 0);
        const int current = rows();
        if (count == current)
            return;
        if (count > current) {
            m_rows.resize(static_cast<std::size_t>(count),
                          std::vector<T>(static_cast<std::size_t>(m_columnCount),
                                         defaultValue));
        } else {
            m_rows.resize(static_cast<std::size_t>(count));
        }
    }

    void insert_columns(int column, int count, const T &value)
    {
        Q_ASSERT(column >= 0 && column <= m_columnCount);
        Q_ASSERT(count >= 0);
        if (count == 0)
            return;
        for (auto &row : m_rows)
            row.insert(row.begin() + column,
                       static_cast<std::size_t>(count), value);
        m_columnCount += count;
    }

    void insert_columns(int column, std::vector<std::vector<T>> colsData)
    {
        Q_ASSERT(column >= 0 && column <= m_columnCount);
#ifdef QT_DEBUG
        for (const auto &c : colsData)
            Q_ASSERT(static_cast<int>(c.size()) == rows());
#endif
        const int count = static_cast<int>(colsData.size());
        if (count == 0)
            return;

        const std::size_t rowCount = m_rows.size();
        for (std::size_t r = 0; r < rowCount; ++r) {
            std::vector<T> cells;
            cells.reserve(static_cast<std::size_t>(count));
            for (int k = 0; k < count; ++k)
                cells.push_back(std::move(
                    colsData[static_cast<std::size_t>(k)][r]));
            auto &rowVec = m_rows[r];
            rowVec.insert(rowVec.begin() + column,
                          std::make_move_iterator(cells.begin()),
                          std::make_move_iterator(cells.end()));
        }
        m_columnCount += count;
    }

    void erase_columns(int first, int last)
    {
        Q_ASSERT(first >= 0 && last >= first && last < m_columnCount);
        for (auto &row : m_rows)
            row.erase(row.begin() + first, row.begin() + last + 1);
        m_columnCount -= (last - first + 1);
    }

    void resize_columns(int count, const T &defaultValue = T())
    {
        Q_ASSERT(count >= 0);
        if (count == m_columnCount)
            return;
        if (count > m_columnCount) {
            for (auto &row : m_rows)
                row.resize(static_cast<std::size_t>(count), defaultValue);
        } else {
            for (auto &row : m_rows)
                row.resize(static_cast<std::size_t>(count));
        }
        m_columnCount = count;
    }

    void reset(std::vector<std::vector<T>> newRows, int columnCount)
    {
        Q_ASSERT(columnCount >= 0);
#ifdef QT_DEBUG
        for (const auto &r : newRows)
            Q_ASSERT(static_cast<int>(r.size()) == columnCount);
#endif
        m_rows = std::move(newRows);
        m_columnCount = columnCount;
    }

private:
    std::vector<std::vector<T>> m_rows;
    int m_columnCount = 0;
};

}  // namespace cute
