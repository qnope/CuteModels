#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

template <typename T>
class BasicTableModel : public BaseModel<T>
{
public:
    explicit BasicTableModel(QObject *parent = nullptr) : BaseModel<T>(parent) {}

    BasicTableModel(int rows, int columns, QObject *parent = nullptr)
        : BasicTableModel(rows, columns, T(), parent)
    {}

    BasicTableModel(int rows, int columns, const T &defaultValue,
                    QObject *parent = nullptr)
        : BaseModel<T>(parent)
        , m_columnCount(columns < 0 ? 0 : columns)
    {
        const int rowsClamped = rows < 0 ? 0 : rows;
        m_rows.assign(static_cast<std::size_t>(rowsClamped),
                      std::vector<T>(static_cast<std::size_t>(m_columnCount),
                                     defaultValue));
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_columnCount;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return {};
        if (row < 0 || row >= static_cast<int>(m_rows.size()))
            return {};
        if (column < 0 || column >= m_columnCount)
            return {};
        return this->createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

    int rows() const noexcept { return static_cast<int>(m_rows.size()); }
    int columns() const noexcept { return m_columnCount; }
    bool empty() const noexcept { return m_rows.empty() || m_columnCount == 0; }

    void push_back_row(T defaultValue = T())
    {
        insert_row(rows(), std::move(defaultValue));
    }

    void insert_row(int row, T defaultValue = T())
    {
        insert_rows(row, 1, std::move(defaultValue));
    }

    void insert_rows(int row, int count, T defaultValue = T())
    {
        if (count <= 0 || row < 0 || row > rows())
            return;
        this->beginInsertRows(QModelIndex(), row, row + count - 1);
        std::vector<T> newRow(static_cast<std::size_t>(m_columnCount),
                              std::move(defaultValue));
        m_rows.insert(m_rows.begin() + row,
                      static_cast<std::size_t>(count), std::move(newRow));
        this->endInsertRows();
    }

    bool insertRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (row < 0 || row > rows() || count < 0)
            return false;
        if (count == 0)
            return true;

        if constexpr (std::is_default_constructible_v<T>) {
            insert_rows(row, count);
            return true;
        } else {
            return false;
        }
    }

    // Insert a single row whose cells take the supplied per-column values.
    bool append_range_row(const std::vector<T> &values)
    {
        return insert_range_row(rows(), values);
    }

    bool insert_range_row(int row, const std::vector<T> &values)
    {
        return insert_range_row(row, std::vector<std::vector<T>>{values});
    }

    // Insert several rows at once, each row carrying its own per-column values.
    bool append_range_row(const std::vector<std::vector<T>> &newRows)
    {
        return insert_range_row(rows(), newRows);
    }

    bool insert_range_row(int row, const std::vector<std::vector<T>> &newRows)
    {
        if (newRows.empty() || row < 0 || row > rows())
            return false;

        int max_newrows_width = 0;
        for (const auto &candidate : newRows)
            max_newrows_width = std::max(max_newrows_width,
                                         static_cast<int>(candidate.size()));

        const int effective_width = std::max(max_newrows_width, m_columnCount);
        const bool need_extend_table = (effective_width > m_columnCount);
        bool need_pad_newrows = false;
        for (const auto &candidate : newRows) {
            if (static_cast<int>(candidate.size()) < effective_width) {
                need_pad_newrows = true;
                break;
            }
        }

        if constexpr (!std::is_default_constructible_v<T>) {
            if (need_extend_table || need_pad_newrows)
                return false;
            const int count = static_cast<int>(newRows.size());
            this->beginInsertRows(QModelIndex(), row, row + count - 1);
            m_rows.insert(m_rows.begin() + row, newRows.begin(), newRows.end());
            this->endInsertRows();
            return true;
        } else {
            if (need_extend_table) {
                this->beginInsertColumns(QModelIndex(),
                                         m_columnCount, effective_width - 1);
                const std::size_t delta = static_cast<std::size_t>(
                    effective_width - m_columnCount);
                for (auto &existing_row : m_rows)
                    existing_row.insert(existing_row.end(), delta, T());
                m_columnCount = effective_width;
                this->endInsertColumns();
            }

            std::vector<std::vector<T>> padded;
            padded.reserve(newRows.size());
            for (const auto &candidate : newRows) {
                padded.push_back(candidate);
                auto &pr = padded.back();
                if (static_cast<int>(pr.size()) < effective_width)
                    pr.resize(static_cast<std::size_t>(effective_width), T());
            }

            const int count = static_cast<int>(padded.size());
            this->beginInsertRows(QModelIndex(), row, row + count - 1);
            m_rows.insert(m_rows.begin() + row,
                          std::make_move_iterator(padded.begin()),
                          std::make_move_iterator(padded.end()));
            this->endInsertRows();
            return true;
        }
    }

    void erase_row(int row) { erase_rows(row, row); }

    void erase_rows(int first, int last)
    {
        if (first < 0 || last < first || last >= rows())
            return;
        this->beginRemoveRows(QModelIndex(), first, last);
        m_rows.erase(m_rows.begin() + first, m_rows.begin() + last + 1);
        this->endRemoveRows();
    }

    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (row < 0 || count < 0 || row + count > rows())
            return false;
        if (count == 0)
            return true;

        erase_rows(row, row + count - 1);
        return true;
    }

    void resize_rows(int count, T defaultValue = T())
    {
        const int current = rows();
        if (count < 0 || count == current)
            return;
        if (count > current) {
            this->beginInsertRows(QModelIndex(), current, count - 1);
            m_rows.resize(static_cast<std::size_t>(count),
                          std::vector<T>(static_cast<std::size_t>(m_columnCount),
                                         std::move(defaultValue)));
            this->endInsertRows();
        } else {
            this->beginRemoveRows(QModelIndex(), count, current - 1);
            m_rows.resize(static_cast<std::size_t>(count));
            this->endRemoveRows();
        }
    }

    void push_back_column(T defaultValue = T())
    {
        insert_column(columns(), std::move(defaultValue));
    }

    void insert_column(int column, T defaultValue = T())
    {
        insert_columns(column, 1, std::move(defaultValue));
    }

    void insert_columns(int column, int count, T defaultValue = T())
    {
        if (count <= 0 || column < 0 || column > m_columnCount)
            return;
        this->beginInsertColumns(QModelIndex(), column, column + count - 1);
        for (auto &row : m_rows)
            row.insert(row.begin() + column,
                       static_cast<std::size_t>(count), defaultValue);
        m_columnCount += count;
        this->endInsertColumns();
    }

    bool insertColumns(int column, int count,
                       const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (column < 0 || column > columns() || count < 0)
            return false;
        if (count == 0)
            return true;

        if constexpr (std::is_default_constructible_v<T>) {
            insert_columns(column, count);
            return true;
        } else {
            return false;
        }
    }

    // Insert a single column whose cells take the supplied per-row values.
    bool append_range_column(const std::vector<T> &values)
    {
        return insert_range_column(columns(), values);
    }

    bool insert_range_column(int column, const std::vector<T> &values)
    {
        return insert_range_column(column, std::vector<std::vector<T>>{values});
    }

    // Insert several columns at once, each column carrying its own per-row values.
    bool append_range_column(const std::vector<std::vector<T>> &newColumns)
    {
        return insert_range_column(columns(), newColumns);
    }

    bool insert_range_column(int column, const std::vector<std::vector<T>> &newColumns)
    {
        if (newColumns.empty() || column < 0 || column > m_columnCount)
            return false;

        int max_newcols_height = 0;
        for (const auto &candidate : newColumns)
            max_newcols_height = std::max(max_newcols_height,
                                          static_cast<int>(candidate.size()));

        const int current_height = rows();
        const int effective_height = std::max(max_newcols_height, current_height);
        const bool need_extend_table = (effective_height > current_height);
        bool need_pad_newcols = false;
        for (const auto &candidate : newColumns) {
            if (static_cast<int>(candidate.size()) < effective_height) {
                need_pad_newcols = true;
                break;
            }
        }

        const int count = static_cast<int>(newColumns.size());

        if constexpr (!std::is_default_constructible_v<T>) {
            if (need_extend_table || need_pad_newcols)
                return false;
            this->beginInsertColumns(QModelIndex(), column, column + count - 1);
            for (std::size_t r = 0; r < m_rows.size(); ++r) {
                std::vector<T> cells;
                cells.reserve(static_cast<std::size_t>(count));
                for (int k = 0; k < count; ++k)
                    cells.push_back(newColumns[static_cast<std::size_t>(k)][r]);
                auto &rowVec = m_rows[r];
                rowVec.insert(rowVec.begin() + column,
                              std::make_move_iterator(cells.begin()),
                              std::make_move_iterator(cells.end()));
            }
            m_columnCount += count;
            this->endInsertColumns();
            return true;
        } else {
            if (need_extend_table) {
                this->beginInsertRows(QModelIndex(),
                                      current_height, effective_height - 1);
                const int delta = effective_height - current_height;
                for (int i = 0; i < delta; ++i)
                    m_rows.push_back(std::vector<T>(
                        static_cast<std::size_t>(m_columnCount), T()));
                this->endInsertRows();
            }

            std::vector<std::vector<T>> padded;
            padded.reserve(newColumns.size());
            for (const auto &candidate : newColumns) {
                padded.push_back(candidate);
                auto &pc = padded.back();
                if (static_cast<int>(pc.size()) < effective_height)
                    pc.resize(static_cast<std::size_t>(effective_height), T());
            }

            this->beginInsertColumns(QModelIndex(), column, column + count - 1);
            for (std::size_t r = 0; r < m_rows.size(); ++r) {
                std::vector<T> cells;
                cells.reserve(static_cast<std::size_t>(count));
                for (int k = 0; k < count; ++k)
                    cells.push_back(std::move(
                        padded[static_cast<std::size_t>(k)][r]));
                auto &rowVec = m_rows[r];
                rowVec.insert(rowVec.begin() + column,
                              std::make_move_iterator(cells.begin()),
                              std::make_move_iterator(cells.end()));
            }
            m_columnCount += count;
            this->endInsertColumns();
            return true;
        }
    }

    void erase_column(int column) { erase_columns(column, column); }

    void erase_columns(int first, int last)
    {
        if (first < 0 || last < first || last >= m_columnCount)
            return;
        this->beginRemoveColumns(QModelIndex(), first, last);
        for (auto &row : m_rows)
            row.erase(row.begin() + first, row.begin() + last + 1);
        m_columnCount -= (last - first + 1);
        this->endRemoveColumns();
    }

    bool removeColumns(int column, int count,
                       const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (column < 0 || count < 0 || column + count > columns())
            return false;
        if (count == 0)
            return true;

        erase_columns(column, column + count - 1);
        return true;
    }

    void resize_columns(int count, T defaultValue = T())
    {
        const int current = m_columnCount;
        if (count < 0 || count == current)
            return;
        if (count > current) {
            this->beginInsertColumns(QModelIndex(), current, count - 1);
            for (auto &row : m_rows)
                row.resize(static_cast<std::size_t>(count), defaultValue);
            m_columnCount = count;
            this->endInsertColumns();
        } else {
            this->beginRemoveColumns(QModelIndex(), count, current - 1);
            for (auto &row : m_rows)
                row.resize(static_cast<std::size_t>(count));
            m_columnCount = count;
            this->endRemoveColumns();
        }
    }

    void clear() { (void)reset({}); }

    bool reset(std::vector<std::vector<T>> newRows)
    {
        int max_width = 0;
        for (const auto &row : newRows)
            max_width = std::max(max_width, static_cast<int>(row.size()));

        bool need_pad = false;
        for (const auto &row : newRows) {
            if (static_cast<int>(row.size()) < max_width) {
                need_pad = true;
                break;
            }
        }

        if constexpr (!std::is_default_constructible_v<T>) {
            if (need_pad)
                return false;
        } else {
            if (need_pad) {
                for (auto &row : newRows) {
                    if (static_cast<int>(row.size()) < max_width)
                        row.resize(static_cast<std::size_t>(max_width), T());
                }
            }
        }

        this->beginResetModel();
        m_rows = std::move(newRows);
        m_columnCount = max_width;
        this->endResetModel();
        return true;
    }

    ItemProxy<const T> at(int row, int column) const
    {
        const QModelIndex idx = this->index(row, column);
        return ItemProxy<const T>(
            m_rows[static_cast<std::size_t>(row)]
                  [static_cast<std::size_t>(column)],
            idx, idx, idx);
    }

    ItemProxy<T> getMutable(int row, int column)
    {
        const QModelIndex idx = this->index(row, column);
        return ItemProxy<T>(
            m_rows[static_cast<std::size_t>(row)]
                  [static_cast<std::size_t>(column)],
            idx, idx, idx);
    }

protected:
    const T &getStorageValue(const QModelIndex &index) const override
    {
        return m_rows[static_cast<std::size_t>(index.row())]
                     [static_cast<std::size_t>(index.column())];
    }

    void setStorageValue(const QModelIndex &index, T value) override
    {
        m_rows[static_cast<std::size_t>(index.row())]
              [static_cast<std::size_t>(index.column())] = std::move(value);
        emit this->dataChanged(index, index);
    }

private:
    std::vector<std::vector<T>> m_rows;
    int m_columnCount = 0;
};

}
