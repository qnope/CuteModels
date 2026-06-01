#pragma once

#include "CuteModel/RefBase.h"
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
#include <memory>
#include <type_traits>
#include <utility>

namespace cute {

// Abstract, header-only base for typed CuteModel models. Like the nested
// BaseModel<T>::Ref over RefBase, BaseModel<T> does NOT carry its own Q_OBJECT
// (moc cannot process class templates) — it relies entirely on the signals
// already declared on QAbstractItemModel (dataChanged, modelReset, …).
//
// Responsibilities:
//   * Strict-contract setData. The only accepted write role is ValueRole —
//     anything else (Qt::EditRole, Qt::DisplayRole, custom roles) is a programming
//     error and terminates. The write — and the dataChanged notification that
//     goes with it — is delegated to the protected setStorageValue(index,
//     value) virtual, which structural subclasses implement against their own
//     storage. setData itself only validates the role, index, and value.
//
//   * Value-based drag source. Subclasses override mimeDataForValue(T) to
//     serialize the dragged value to a mime payload; mimeData() reads the
//     single dragged index through ValueRole and hands the value off. Drop
//     handling is deliberately NOT a BaseModel concern — where a drop lands
//     (which element it targets, or where it inserts) depends on the model's
//     structure, so it lives on the structural subclasses (e.g.
//     BasicListModel's dropOnElement / dropInsertion and their canDrop
//     counterparts).
//
//   * roleNames advertising ValueRole, so QML can bind "value".
//
// Subclasses expose their storage to the read/edit machinery through two typed
// hooks — getStorageValue (a const T& straight into storage) and
// setStorageValue (write + dataChanged). BaseModel and its nested Ref are built
// on top of those, so they never need to round-trip a value through QVariant.
// Structural subclasses (BasicListModel, BasicTableModel, BasicTreeModel, …)
// own their storage and the multiData / data / flags overrides that project it
// to roles.
template <typename T>
class BaseModel : public QAbstractItemModel
{
    static_assert(std::is_copy_constructible_v<T> && std::is_destructible_v<T>,
                  "BaseModel<T> requires T to be copy-constructible and "
                  "destructible so it can round-trip through QVariant via "
                  "QVariant::fromValue<T>.");

public:
    using QAbstractItemModel::QAbstractItemModel;

    // ---------- Ref ----------

    // Typed view over a single model item, nested inside the model it belongs
    // to. It keeps the owning BaseModel<T>* and a QPersistentModelIndex, and
    // reads/writes the item straight through the model's typed storage hooks
    // (getStorageValue / setStorageValue) — no QVariant round-trip.
    //
    // No Q_OBJECT here: the change signals are inherited from RefBase (moc
    // cannot process class templates). Refs are not constructed directly:
    // BaseModel<T> is the sole factory (BaseModel::getRef), so the constructor
    // is protected and BaseModel is a friend.
    class Ref : public RefBase
    {
    public:
        // Read access to the item's stored value. Returns a const reference
        // straight into the model's storage via getStorageValue. An invalid
        // index — or one whose model has gone away — is a programming error and
        // terminates.
        const T &getValue() const
        {
            if (!m_index.isValid() || !m_model)
                std::terminate();

            return m_model->getStorageValue(m_index);
        }

        // Writes `value` back into the model item through setStorageValue,
        // which performs the write and emits dataChanged. The write cannot
        // fail, so there is nothing to report. An invalid index — or one whose
        // model has gone away — is a programming error and terminates,
        // mirroring getValue.
        void setValue(const T &value)
        {
            if (!m_index.isValid() || !m_model)
                std::terminate();

            m_model->setStorageValue(m_index, value);
        }

    protected:
        Ref(BaseModel *model, QPersistentModelIndex index)
            : RefBase(std::move(index))
            , m_model(model)
        {}

        BaseModel *m_model;

        friend class BaseModel;
    };

    // ---------- Ref factory ----------

    // Build a Ref that follows `index`. Ownership is handed to the caller
    // through the returned std::unique_ptr — the Ref is not parented to the
    // model. Defaults to the nested Ref; pass a concrete Ref subclass to get a
    // typed view. Returns nullptr for an invalid index.
    //
    // BaseModel<T> is the sole factory for Ref (its constructor is protected
    // and befriends BaseModel), which is why this lives here rather than in a
    // free function.
    template <typename R = Ref>
    std::unique_ptr<R> getRef(const QModelIndex &index)
    {
        static_assert(std::is_base_of_v<Ref, R>,
                      "getRef<R> requires R to derive from BaseModel<T>::Ref");

        if (!index.isValid())
            return nullptr;

        return std::unique_ptr<R>(new R(this, QPersistentModelIndex(index)));
    }

    // ---------- Drag source, value-based ----------

    // Serialize a value to a mime payload. Default: nullptr (drag disabled).
    virtual QMimeData *mimeDataForValue(const T &value) const { return nullptr; }

    // ---------- QAbstractItemModel overrides ----------

    // Strict role contract: only ValueRole is accepted. Anything else
    // (including Qt::EditRole, Qt::DisplayRole, custom roles) is a programming
    // error and terminates. The write — and the dataChanged emission — is
    // delegated to setStorageValue.
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = ValueRole) override
    {
        if (role != ValueRole)
            std::terminate();
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return false;
        if (!value.canConvert<T>())
            return false;

        setStorageValue(index, value.value<T>());
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
        names.insert(ValueRole, QByteArrayLiteral("value"));
        return names;
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

protected:
    // Read the value the model stores at `index`, returned by const reference
    // straight out of the subclass's storage (no QVariant round-trip). This is
    // the typed read hook behind Ref::getValue. `index` is guaranteed valid by
    // the callers.
    virtual const T &getStorageValue(const QModelIndex &index) const = 0;

    // Write `value` to the model's underlying storage at `index` and emit the
    // appropriate dataChanged notification. This is the single write+notify
    // hook used by setData and Ref::setValue; the ItemProxy<T> path (owned by
    // subclasses) bypasses it, mutating the reference directly and emitting
    // dataChanged from its own destructor.
    virtual void setStorageValue(const QModelIndex &index, T value) = 0;
};

} // namespace cute
