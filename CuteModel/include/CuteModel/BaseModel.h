#pragma once

#include "CuteModel/ItemProxy.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPersistentModelIndex>
#include <QVariant>

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace cute {

// Abstract, header-only base for typed CuteModel models. Like Ref<T> over
// RefBase, BaseModel<T> does NOT carry its own Q_OBJECT (moc cannot process
// class templates) — it relies entirely on the signals already declared on
// QAbstractItemModel (dataChanged, modelReset, …).
//
// Responsibilities:
//   * Single-cell storage contract via two protected virtuals:
//       getStorageValue(index) -> const T &      (fetch from storage)
//       setStorageValue(index, value)            (write to storage)
//     so subclasses (BasicListModel, BasicTableModel, BasicTreeModel, …)
//     decide their own row/column/tree layout while sharing the value plumbing.
//
//   * Strict-contract setData. The only accepted write role is Qt::EditRole —
//     anything else (Qt::DisplayRole, ValueRole, custom roles) is a programming
//     error and terminates. Writes go directly through setStorageValue; no
//     edit cache lives at this layer. A column-aware cache will come back in
//     a later PR alongside BasicTableModel<T>, where two setData calls on two
//     columns of the same row need to merge into a single pending T.
//
//   * Value-based drag/drop. Subclasses override mimeDataForValue(T),
//     canDropValue(...), and dropValue(...) and see the parent VALUE
//     (std::optional<T>, nullopt at root) plus the raw mime data, instead
//     of a parent QModelIndex.
//
// Role projection (the data(const T&, int) / flags(const T&) pair) is NOT
// part of this layer because table/tree-shaped models need wider signatures
// (column-aware). Structural subclasses own the multiData / data /
// flags(QModelIndex) overrides and define their own projection contract;
// BaseModel only exposes getStorageValue() so those overrides can read the
// underlying value.
template <typename T>
class BaseModel : public QAbstractItemModel
{
    static_assert(std::is_copy_constructible_v<T> && std::is_destructible_v<T>,
                  "BaseModel<T> requires T to be copy-constructible and "
                  "destructible so it can round-trip through QVariant via "
                  "QVariant::fromValue<T>.");

public:
    using QAbstractItemModel::QAbstractItemModel;

    // ---------- Drag/drop, value-based ----------

    // Serialize a value to a mime payload. Default: nullptr (drag disabled).
    virtual QMimeData *mimeDataForValue(const T &value) const { return nullptr; }

    // canDropValue / dropValue: subclasses see the parent VALUE (not index) and
    // raw mime payload.
    //   parent == std::nullopt  -> drop targets the root
    //   row    == std::nullopt  -> drop targets the parent item itself
    //                              (Qt's "row == -1" convention)
    // Default behavior: refuse everything.
    virtual bool canDropValue(std::optional<T> parent,
                              std::optional<int> row,
                              int column,
                              Qt::DropAction action,
                              const QMimeData *data) const
    {
        return false;
    }

    virtual bool dropValue(std::optional<T> parent,
                           std::optional<int> row,
                           int column,
                           Qt::DropAction action,
                           const QMimeData *data)
    {
        return false;
    }

    // ---------- QAbstractItemModel overrides ----------

    // Strict role contract: only Qt::EditRole is accepted. Anything else
    // (including Qt::DisplayRole, ValueRole, custom roles) is a programming
    // error and terminates.
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole)
            std::terminate();
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return false;
        if (!value.canConvert<T>())
            return false;

        setStorageValue(index, value.value<T>());
        emit dataChanged(index, index); // empty roles list = "all roles changed"
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
        names.insert(ValueRole, QByteArrayLiteral("value"));
        return names;
    }

    bool canDropMimeData(const QMimeData *data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex &parent) const override
    {
        return canDropValue(valueOrOptional(parent), rowOptional(row), column, action, data);
    }

    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override
    {
        return dropValue(valueOrOptional(parent), rowOptional(row), column, action, data);
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        if (indexes.size() != 1)
            return nullptr;
        const QModelIndex &index = indexes.front();
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return nullptr;
        return mimeDataForValue(getStorageValue(index));
    }

    // ---------- Proxy accessors (QModelIndex-keyed) ----------

    ItemProxy<const T> at(const QModelIndex &index) const
    {
        return ItemProxy<const T>(getStorageValue(index), QPersistentModelIndex(index));
    }

    ItemProxy<T> getMutable(const QModelIndex &index)
    {
        return ItemProxy<T>(getStorageValue(index), QPersistentModelIndex(index));
    }

protected:
    // Storage contract — subclasses MUST implement both overloads. The
    // const overload feeds reads (multiData, flags, mimeData, …); the
    // mutable overload feeds ItemProxy<T>'s direct in-place writes.
    virtual const T &getStorageValue(const QModelIndex &index) const = 0;
    virtual T &getStorageValue(const QModelIndex &index) = 0;
    // Write `value` to the model's underlying storage at `index`. Used by
    // setData; the ItemProxy<T> path bypasses this and mutates the
    // reference directly.
    virtual void setStorageValue(const QModelIndex &index, T value) = 0;

private:
    // "invalid index -> nullopt" wrapper around getStorageValue, used by the
    // value-based drop adapters to translate the parent QModelIndex argument.
    std::optional<T> valueOrOptional(const QModelIndex &index) const
    {
        if (!index.isValid())
            return std::nullopt;
        return getStorageValue(index);
    }

    static std::optional<int> rowOptional(int row)
    {
        if (row < 0)
            return std::nullopt;
        return row;
    }
};

} // namespace cute
