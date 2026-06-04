#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

#include <cstddef>
#include <exception>
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
            std::terminate();
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
            std::terminate();
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

    void clear() { reset({}); }

    void reset(std::vector<std::vector<T>> newRows)
    {
        int newColumnCount = 0;
        if (!newRows.empty()) {
            newColumnCount = static_cast<int>(newRows.front().size());
            for (const auto &row : newRows) {
                if (static_cast<int>(row.size()) != newColumnCount)
                    std::terminate();
            }
        }
        this->beginResetModel();
        m_rows = std::move(newRows);
        m_columnCount = newColumnCount;
        this->endResetModel();
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
