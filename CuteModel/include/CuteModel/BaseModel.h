#pragma once

#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QVariant>

#include <optional>
#include <type_traits>

namespace cute {

// Abstract, header-only base for typed CuteModel models. Like Ref<T> over
// RefBase, BaseModel<T> does NOT carry its own Q_OBJECT (moc cannot process
// class templates) — it relies entirely on the signals already declared on
// QAbstractItemModel (dataChanged, modelReset, …).
//
// Responsibilities:
//   * Value-based drag/drop. Subclasses override mimeDataForValue(T),
//     canDropValue(...), and dropValue(...) and see the parent VALUE
//     (std::optional<T>, nullopt at root) plus the raw mime data, instead
//     of a parent QModelIndex.
//
//   * roleNames advertising ValueRole, so QML can bind "value".
//
// BaseModel does NOT own any storage. It reads cell values through the public
// role path — index.data(ValueRole).value<T>() — so it never needs to know
// how a subclass lays out its rows/columns/tree. Structural subclasses
// (BasicListModel, BasicTableModel, BasicTreeModel, …) own their storage and
// the multiData / data / flags / setData overrides that project it to roles;
// they are also the ones that expose T-reference accessors (ItemProxy) and the
// write path, because those cannot round-trip through QVariant.
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
        return mimeDataForValue(index.data(ValueRole).value<T>());
    }

private:
    // "invalid index -> nullopt" wrapper around the public ValueRole read,
    // used by the value-based drop adapters to translate the parent QModelIndex
    // argument.
    std::optional<T> valueOrOptional(const QModelIndex &index) const
    {
        if (!index.isValid())
            return std::nullopt;
        return index.data(ValueRole).value<T>();
    }

    static std::optional<int> rowOptional(int row)
    {
        if (row < 0)
            return std::nullopt;
        return row;
    }
};

} // namespace cute
