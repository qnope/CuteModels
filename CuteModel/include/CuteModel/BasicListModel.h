#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QModelIndex>
#include <QModelRoleData>
#include <QVariant>

#include <cstddef>
#include <utility>
#include <vector>

namespace cute {

// Flat, in-memory list model backed by a std::vector<T>. Inherits the storage
// contract (getValue/setValue), edit cache, and drag/drop adapters from
// BaseModel<T>; provides the list-shaped structural overrides, the
// row-only role projection, and the std::vector-flavored container API.
template <typename T>
class BasicListModel : public BaseModel<T>
{
public:
    using BaseModel<T>::BaseModel;
    using BaseModel<T>::at;
    using BaseModel<T>::getMutable;

    using const_iterator = typename std::vector<T>::const_iterator;

    // ---------- Role projection (row-only — list-specific) ----------
    //
    // Subclasses describe how a stored value maps to roles other than
    // ValueRole; return an invalid QVariant for unhandled roles. Table/tree
    // models will need a wider signature (column-aware), which is why these
    // virtuals live here rather than on BaseModel.

    virtual QVariant data(const T &value, int role) const = 0;

    virtual Qt::ItemFlags flags(const T &value) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    // ---------- Structural overrides ----------

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_items.size());
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || column != 0)
            return {};
        if (row < 0 || row >= static_cast<int>(m_items.size()))
            return {};
        return this->createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

    // ---------- Read path ----------

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QModelRoleData roleData(role);
        multiData(index, roleData);
        return roleData.data();
    }

    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override
    {
        if (!this->checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid))
            return;

        const T &value = m_items[static_cast<std::size_t>(index.row())];
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == ValueRole) {
                roleData.setData(QVariant::fromValue(value));
            } else if (QVariant projected = data(value, roleData.role()); projected.isValid()) {
                roleData.data() = std::move(projected);
            } else {
                roleData.clearData();
            }
        }
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return flags(this->getStorageValue(index));
    }

    // ---------- Container API ----------

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

    // ---------- Ergonomic int-row accessors (cache-aware reads) ----------

    ItemProxy<const T> at(int row) const
    {
        return BaseModel<T>::at(this->index(row, 0));
    }

    ItemProxy<const T> operator[](int row) const
    {
        return BaseModel<T>::at(this->index(row, 0));
    }

    ItemProxy<T> getMutable(int row)
    {
        return BaseModel<T>::getMutable(this->index(row, 0));
    }

    // ---------- Raw-storage iteration (bypasses the cache) ----------

    const_iterator begin() const noexcept { return m_items.begin(); }
    const_iterator end() const noexcept { return m_items.end(); }
    const_iterator cbegin() const noexcept { return m_items.cbegin(); }
    const_iterator cend() const noexcept { return m_items.cend(); }

protected:
    const T &getStorageValue(const QModelIndex &index) const override
    {
        return m_items[static_cast<std::size_t>(index.row())];
    }

    T &getStorageValue(const QModelIndex &index) override
    {
        return m_items[static_cast<std::size_t>(index.row())];
    }

    void setStorageValue(const QModelIndex &index, T value) override
    {
        m_items[static_cast<std::size_t>(index.row())] = std::move(value);
    }

private:
    std::vector<T> m_items;
};

} // namespace cute
