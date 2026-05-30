#pragma once

#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QVariant>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

// An in-memory, generic list model backed by a std::vector<T>.
//
// Like Ref<T> over RefBase, this is a header-only template that does NOT carry
// its own Q_OBJECT (moc cannot process class templates). It declares no new
// signals or slots: it emits the inherited dataChanged and uses the protected
// begin/end row helpers QAbstractListModel already provides.
//
// The class is abstract: a concrete model implements data(const T&, int) to map
// a stored value onto presentation roles. The ValueRole contract that Ref<T>
// depends on is guaranteed by the base (see multiData), so subclasses only need
// to describe the roles they care about (e.g. Qt::DisplayRole).
template <typename T>
class BasicListModel : public QAbstractListModel
{
    static_assert(std::is_copy_constructible_v<T> && std::is_destructible_v<T>,
                  "BasicListModel<T> requires T to be copy-constructible and "
                  "destructible so it can round-trip through QVariant via "
                  "QVariant::fromValue<T>.");

public:
    using QAbstractListModel::QAbstractListModel;

    using const_iterator = typename std::vector<T>::const_iterator;

    // Role projection: subclasses describe how a stored value maps to roles
    // other than ValueRole. Return an invalid QVariant for unhandled roles.
    virtual QVariant data(const T &value, int role) const = 0;

    virtual Qt::ItemFlags flags(const T &value) const { return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_items.size());
    }

    // Single source of truth for reads. ValueRole returns the value itself so
    // Ref<T>::getValue() always round-trips; every other role is delegated to
    // the subclass projection.
    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
            return;

        const T &value = m_items[static_cast<std::size_t>(index.row())];
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == ValueRole) {
                roleData.setData(value);
            } else if (QVariant projected = data(value, roleData.role()); projected.isValid()) {
                roleData.data() = std::move(projected);
            } else {
                roleData.clearData();
            }
        }
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QModelRoleData roleData(role);
        multiData(index, roleData);
        return roleData.data();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return flags(m_items[static_cast<std::size_t>(index.row())]);
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
            return false;
        if (role != ValueRole && role != Qt::EditRole && role != Qt::DisplayRole)
            return false;
        if (!value.canConvert<T>())
            return false;

        m_items[static_cast<std::size_t>(index.row())] = value.value<T>();
        emit dataChanged(index, index, {ValueRole, Qt::DisplayRole, Qt::EditRole});
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> names = QAbstractListModel::roleNames();
        names.insert(ValueRole, QByteArrayLiteral("value"));
        return names;
    }

    int size() const noexcept { return static_cast<int>(m_items.size()); }
    bool empty() const noexcept { return m_items.empty(); }

    const T &at(int row) const { return m_items.at(static_cast<std::size_t>(row)); }

    void push_back(T value)
    {
        const int row = size();
        beginInsertRows(QModelIndex(), row, row);
        m_items.push_back(std::move(value));
        endInsertRows();
    }

    template <typename... Args>
    T &emplace_back(Args &&...args)
    {
        const int row = size();
        beginInsertRows(QModelIndex(), row, row);
        T &ref = m_items.emplace_back(std::forward<Args>(args)...);
        endInsertRows();
        return ref;
    }

    void insert(int row, T value)
    {
        beginInsertRows(QModelIndex(), row, row);
        m_items.insert(m_items.begin() + row, std::move(value));
        endInsertRows();
    }

    void append_range(const std::vector<T> &values)
    {
        if (values.empty())
            return;
        const int first = size();
        const int last = first + static_cast<int>(values.size()) - 1;
        beginInsertRows(QModelIndex(), first, last);
        m_items.insert(m_items.end(), values.begin(), values.end());
        endInsertRows();
    }

    void insert_range(int row, const std::vector<T> &values)
    {
        if (values.empty())
            return;
        const int last = row + static_cast<int>(values.size()) - 1;
        beginInsertRows(QModelIndex(), row, last);
        m_items.insert(m_items.begin() + row, values.begin(), values.end());
        endInsertRows();
    }

    void erase(int row) { erase(row, row); }

    void erase(int first, int last)
    {
        if (first < 0 || last < first || last >= size())
            return;
        beginRemoveRows(QModelIndex(), first, last);
        m_items.erase(m_items.begin() + first, m_items.begin() + last + 1);
        endRemoveRows();
    }

    void clear()
    {
        reset({});
    }

    void reset(std::vector<T> newItems)
    {
        beginResetModel();
        m_items = std::move(newItems);
        endResetModel();
    }

    void resize(int count, T defaultValue = T())
    {
        const int current = size();
        if (count < 0 || count == current)
            return;

        if (count > current) {
            beginInsertRows(QModelIndex(), current, count - 1);
            m_items.resize(static_cast<std::size_t>(count), std::move(defaultValue));
            endInsertRows();
        } else {
            beginRemoveRows(QModelIndex(), count, current - 1);
            m_items.resize(static_cast<std::size_t>(count));
            endRemoveRows();
        }
    }

    // operator[] is read-only: it returns an ItemConstProxy that takes a
    // snapshot of the row's value. Use getMutable(row) when you intend to
    // write — a mutable proxy commits unconditionally on destruction.
    ItemConstProxy<T> operator[](int row) const
    {
        return ItemConstProxy<T>(index(row, 0));
    }

    ItemProxy<T> getMutable(int row)
    {
        return ItemProxy<T>(QPersistentModelIndex(index(row, 0)));
    }

    const_iterator begin() const noexcept { return m_items.begin(); }
    const_iterator end() const noexcept { return m_items.end(); }
    const_iterator cbegin() const noexcept { return m_items.cbegin(); }
    const_iterator cend() const noexcept { return m_items.cend(); }

private:
    std::vector<T> m_items;
};

} // namespace cute
