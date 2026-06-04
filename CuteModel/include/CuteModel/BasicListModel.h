#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ItemProxy.h"
#include "CuteModel/RangeList.h"
#include "CuteModel/ValueRole.h"

#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

template <typename T>
class BasicListModel : public BaseModel<T>
{
public:
    explicit BasicListModel(QObject *parent = nullptr) : BaseModel<T>(parent) {}

    explicit BasicListModel(QStringList headers, QObject *parent = nullptr)
        : BaseModel<T>(parent)
        , m_headers(headers.isEmpty() ? QStringList{QString()} : std::move(headers))
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_items.size());
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_headers.size());
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return {};
        if (column < 0 || column >= static_cast<int>(m_headers.size()))
            return {};
        if (row < 0 || row >= static_cast<int>(m_items.size()))
            return {};
        return this->createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole
            && section >= 0 && section < m_headers.size())
            return m_headers.at(section);
        return QAbstractItemModel::headerData(section, orientation, role);
    }

    int size() const noexcept { return static_cast<int>(m_items.size()); }
    bool empty() const noexcept { return m_items.empty(); }

    void push_back(T value)
    {
        const int row = size();
        this->beginInsertRows(QModelIndex(), row, row);
        m_items.push_back(std::move(value));
        this->endInsertRows();
    }

    template <typename... Args>
    T &emplace_back(Args &&...args)
    {
        const int row = size();
        this->beginInsertRows(QModelIndex(), row, row);
        T &ref = m_items.emplace_back(std::forward<Args>(args)...);
        this->endInsertRows();
        return ref;
    }

    void insert(int row, T value)
    {
        this->beginInsertRows(QModelIndex(), row, row);
        m_items.insert(m_items.begin() + row, std::move(value));
        this->endInsertRows();
    }

    void append_range(const std::vector<T> &values)
    {
        if (values.empty())
            return;
        const int first = size();
        const int last = first + static_cast<int>(values.size()) - 1;
        this->beginInsertRows(QModelIndex(), first, last);
        m_items.insert(m_items.end(), values.begin(), values.end());
        this->endInsertRows();
    }

    void insert_range(int row, const std::vector<T> &values)
    {
        if (values.empty())
            return;
        const int last = row + static_cast<int>(values.size()) - 1;
        this->beginInsertRows(QModelIndex(), row, last);
        m_items.insert(m_items.begin() + row, values.begin(), values.end());
        this->endInsertRows();
    }

    bool insertRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (row < 0 || row > size() || count < 0)
            return false;
        if (count == 0)
            return true;

        if constexpr (std::is_default_constructible_v<T>) {
            const int last = row + count - 1;
            this->beginInsertRows(QModelIndex(), row, last);
            m_items.insert(m_items.begin() + row,
                           static_cast<std::size_t>(count), T());
            this->endInsertRows();
            return true;
        } else {
            std::terminate();
        }
    }

    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        if (parent.isValid())
            return false;
        if (row < 0 || count < 0 || row + count > size())
            return false;
        if (count == 0)
            return true;

        const int last = row + count - 1;
        this->beginRemoveRows(QModelIndex(), row, last);
        m_items.erase(m_items.begin() + row, m_items.begin() + row + count);
        this->endRemoveRows();
        return true;
    }

    void erase(int row) { erase(row, row); }

    void erase(int first, int last)
    {
        if (first < 0 || last < first || last >= size())
            return;
        this->beginRemoveRows(QModelIndex(), first, last);
        m_items.erase(m_items.begin() + first, m_items.begin() + last + 1);
        this->endRemoveRows();
    }

    void clear() { reset({}); }

    void reset(std::vector<T> newItems)
    {
        this->beginResetModel();
        m_items = std::move(newItems);
        this->endResetModel();
    }

    void resize(int count, T defaultValue = T())
    {
        const int current = size();
        if (count < 0 || count == current)
            return;

        if (count > current) {
            this->beginInsertRows(QModelIndex(), current, count - 1);
            m_items.resize(static_cast<std::size_t>(count), std::move(defaultValue));
            this->endInsertRows();
        } else {
            this->beginRemoveRows(QModelIndex(), count, current - 1);
            m_items.resize(static_cast<std::size_t>(count));
            this->endRemoveRows();
        }
    }

    ItemProxy<const T> at(int row) const
    {
        const QModelIndex left = this->index(row, 0);
        const QModelIndex right = this->index(row, this->columnCount() - 1);
        return ItemProxy<const T>(m_items[static_cast<std::size_t>(row)], left, left, right);
    }

    ItemProxy<const T> operator[](int row) const { return at(row); }

    ItemProxy<T> getMutable(int row)
    {
        const QModelIndex left = this->index(row, 0);
        const QModelIndex right = this->index(row, this->columnCount() - 1);
        return ItemProxy<T>(m_items[static_cast<std::size_t>(row)], left, left, right);
    }

    RangeList<const T> iter() const { return iter(0, size()); }

    RangeList<const T> iter(int first, int number) const
    {
        if (first < 0 || number < 0 || first + number > size())
            std::terminate();
        if (number == 0)
            return RangeList<const T>(m_items.cbegin() + first,
                                      m_items.cbegin() + first,
                                      QPersistentModelIndex(),
                                      QPersistentModelIndex());
        return RangeList<const T>(m_items.cbegin() + first,
                                  m_items.cbegin() + first + number,
                                  this->index(first, 0),
                                  this->index(first + number - 1,
                                              this->columnCount() - 1));
    }

    RangeList<T> iter_mut() { return iter_mut(0, size()); }

    RangeList<T> iter_mut(int first, int number)
    {
        if (first < 0 || number < 0 || first + number > size())
            std::terminate();
        if (number == 0)
            return RangeList<T>(m_items.begin() + first,
                                m_items.begin() + first,
                                QPersistentModelIndex(),
                                QPersistentModelIndex());
        return RangeList<T>(m_items.begin() + first,
                            m_items.begin() + first + number,
                            this->index(first, 0),
                            this->index(first + number - 1,
                                        this->columnCount() - 1));
    }

protected:
    const T &getStorageValue(const QModelIndex &index) const override
    {
        return m_items[static_cast<std::size_t>(index.row())];
    }

    void setStorageValue(const QModelIndex &index, T value) override
    {
        m_items[static_cast<std::size_t>(index.row())] = std::move(value);
        const QModelIndex left = this->index(index.row(), 0);
        const QModelIndex right = this->index(index.row(), this->columnCount() - 1);
        emit this->dataChanged(left, right);
    }

private:
    std::vector<T> m_items;
    QStringList m_headers{QString()};
};

}
