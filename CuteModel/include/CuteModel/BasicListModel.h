#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QMimeData>
#include <QModelIndex>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <utility>
#include <vector>

namespace cute {

// Flat, in-memory list model backed by a std::vector<T>: 1D storage but
// N-column-displayable. A QStringList headers constructor argument fixes the
// column count; each row is still one T, and subclasses project that T to
// roles per column via data(const T&, int column, int role).
//
// A row is one whole T, so an edit always rewrites the entire T and refreshes
// every column of that row. Columns > 0 are non-editable by default —
// flags(T&, column) strips ItemIsEditable for col>0 — but the write itself is
// column-agnostic: only the row matters. True 2D storage (independent T per
// cell) is reserved for the future BasicTableModel<T>.
template <typename T>
class BasicListModel : public BaseModel<T>
{
public:
    using const_iterator = typename std::vector<T>::const_iterator;

    // Two ctors declared explicitly (rather than `using BaseModel<T>::BaseModel`)
    // because MSVC flags inherited-ctor + new-ctor combinations as ambiguous
    // during default-construction of subclasses, even when overload resolution
    // is unambiguous on paper.
    explicit BasicListModel(QObject *parent = nullptr) : BaseModel<T>(parent) {}

    explicit BasicListModel(QStringList headers, QObject *parent = nullptr)
        : BaseModel<T>(parent)
        , m_headers(headers.isEmpty() ? QStringList{QString()} : std::move(headers))
    {}

    // ---------- Role projection (per-column — list-specific) ----------
    //
    // Subclasses describe how a stored value maps to roles other than
    // ValueRole, for each column; return an invalid QVariant for unhandled
    // (column, role) pairs.

    virtual QVariant data(const T &value, int column, int role) const = 0;

    virtual Qt::ItemFlags flags(const T &value, int column) const
    {
        Qt::ItemFlags base = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (column == 0)
            base |= Qt::ItemIsEditable;
        return base;
    }

    // ---------- Drop hooks (list-specific) ----------
    //
    // A drop on a flat list resolves to exactly one of two shapes, and the
    // structural dispatch (canDropMimeData / dropMimeData below) feeds the
    // matching hook so subclasses never inspect a raw QModelIndex:
    //
    //   * onElement   — the drop lands ON an existing row. The hook receives
    //                   that row's value by const reference plus the targeted
    //                   column. (Qt's "row == -1, parent valid" convention.)
    //   * insertion   — the drop lands BETWEEN rows (or past the end). The hook
    //                   receives the insertion row. A drop on the empty
    //                   viewport (Qt's "row == -1, parent invalid") is routed
    //                   here as an append, i.e. row == size().
    //
    // Default behavior: refuse everything.

    virtual bool canDropOnElement(const T &element, int column,
                                  Qt::DropAction action, const QMimeData *data) const
    {
        return false;
    }

    virtual bool dropOnElement(const T &element, int column,
                               Qt::DropAction action, const QMimeData *data)
    {
        return false;
    }

    virtual bool canDropInsertion(int row, int column,
                                  Qt::DropAction action, const QMimeData *data) const
    {
        return false;
    }

    virtual bool dropInsertion(int row, int column,
                               Qt::DropAction action, const QMimeData *data)
    {
        return false;
    }

    // ---------- Structural overrides ----------

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
        const int column = index.column();
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == ValueRole) {
                roleData.setData(QVariant::fromValue(value));
            } else if (QVariant projected = data(value, column, roleData.role());
                       projected.isValid()) {
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
        return flags(m_items[static_cast<std::size_t>(index.row())], index.column());
    }

    // ---------- Drop dispatch ----------
    //
    // Translate Qt's (row, column, parent) drop coordinates into the
    // list-shaped onElement / insertion hooks above:
    //   * parent valid            -> onElement (the targeted row's value)
    //   * parent invalid, row >= 0 -> insertion at that row
    //   * parent invalid, row < 0  -> insertion appended at size()

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                         int row, int column, const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return canDropOnElement(m_items[static_cast<std::size_t>(parent.row())],
                                    parent.column(), action, data);
        return canDropInsertion(row < 0 ? size() : row, column, action, data);
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override
    {
        if (parent.isValid())
            return dropOnElement(m_items[static_cast<std::size_t>(parent.row())],
                                 parent.column(), action, data);
        return dropInsertion(row < 0 ? size() : row, column, action, data);
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

    // ---------- Ergonomic int-row accessors ----------
    //
    // ItemProxy holds a reference straight into m_items (no QVariant
    // round-trip). An out-of-range row yields an invalid index, which makes
    // the ItemProxy constructor terminate — callers are expected to pass a
    // live row.

    ItemProxy<const T> at(int row) const
    {
        return ItemProxy<const T>(m_items[static_cast<std::size_t>(row)],
                                  QPersistentModelIndex(this->index(row, 0)));
    }

    ItemProxy<const T> operator[](int row) const { return at(row); }

    ItemProxy<T> getMutable(int row)
    {
        return ItemProxy<T>(m_items[static_cast<std::size_t>(row)],
                            QPersistentModelIndex(this->index(row, 0)));
    }

    // ---------- Raw-storage iteration ----------

    const_iterator begin() const noexcept { return m_items.begin(); }
    const_iterator end() const noexcept { return m_items.end(); }
    const_iterator cbegin() const noexcept { return m_items.cbegin(); }
    const_iterator cend() const noexcept { return m_items.cend(); }

protected:
    // A row is one whole T, so the stored value is addressed by row alone (the
    // column is irrelevant) and returned by const reference straight out of
    // m_items.
    const T &getStorageValue(const QModelIndex &index) const override
    {
        return m_items[static_cast<std::size_t>(index.row())];
    }

    // A row is one whole T, so writing it refreshes every column of the row:
    // emit dataChanged across the full row range (columns 0..columnCount-1),
    // not just the edited cell. Only the row is significant here.
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

} // namespace cute
