#pragma once

#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QPersistentModelIndex>
#include <QVariant>

#include <exception>
#include <memory>
#include <optional>
#include <utility>

namespace cute {

// Read-only view over a single model item. At construction the value is
// retrieved through the abstract QAbstractItemModel::data(ValueRole) interface
// and kept inside the proxy, so dereference returns a stable reference (or
// pointer) into local storage. The proxy is therefore a snapshot: subsequent
// model writes do not refresh it. The QPersistentModelIndex is preserved so
// callers can still observe how the original row moved.
//
// Constructing with an invalid index is permitted (the proxy holds no value);
// dereferencing such a proxy terminates, matching Ref<T>::getValue().
template <typename T>
class ItemConstProxy
{
public:
    explicit ItemConstProxy(QPersistentModelIndex index)
        : m_index(std::move(index))
    {
        if (!m_index.isValid())
            return;
        QVariant variant = m_index.data(ValueRole);
        if (!variant.isValid())
            std::terminate();
        m_value = variant.template value<T>();
    }

    const T &operator*() const
    {
        if (!m_value)
            std::terminate();
        return *m_value;
    }
    const T *operator->() const { return std::addressof(**this); }

    const QPersistentModelIndex &index() const noexcept { return m_index; }

private:
    QPersistentModelIndex m_index;
    std::optional<T> m_value;
};

// Mutable view over a single model item. The value is retrieved into local
// storage at construction just like ItemConstProxy; dereference returns a
// (non-const) reference / pointer so callers can mutate in place — both
// `*ref = x;` and `ref->method();` work, on top of the convenience
// `ref = x;` overload.
//
// On destruction the stored value is ALWAYS committed back through
// setData(ValueRole) — there is no dirty tracking. Acquiring a mutable proxy
// is the explicit signal of intent to write; if you only want to read, use
// the const proxy from operator[]. The proxy never touches storage directly,
// so an edition cache or other layered model is free to intercept the write.
//
// Non-copyable / non-movable so a single proxy maps to at most one setData
// call (and thus one dataChanged notification). If the row was erased before
// destruction, the commit is silently dropped (RAII-safe).
template <typename T>
class ItemProxy
{
public:
    explicit ItemProxy(QPersistentModelIndex index)
        : m_index(std::move(index))
    {
        if (!m_index.isValid())
            return;
        QVariant variant = m_index.data(ValueRole);
        if (!variant.isValid())
            std::terminate();
        m_value = variant.template value<T>();
    }

    ItemProxy(const ItemProxy &) = delete;
    ItemProxy &operator=(const ItemProxy &) = delete;
    ItemProxy(ItemProxy &&) = delete;

    ~ItemProxy()
    {
        if (!m_value || !m_index.isValid())
            return;
        // QPersistentModelIndex::model() returns const* by API convention;
        // setData is non-const. The model owning this index was non-const
        // when the proxy was created, so recovering write access is sound.
        auto *model = const_cast<QAbstractItemModel *>(m_index.model());
        model->setData(m_index, QVariant::fromValue(std::move(*m_value)), ValueRole);
    }

    T &operator*()
    {
        if (!m_value)
            std::terminate();
        return *m_value;
    }
    const T &operator*() const
    {
        if (!m_value)
            std::terminate();
        return *m_value;
    }
    T *operator->() { return std::addressof(**this); }
    const T *operator->() const { return std::addressof(**this); }

    ItemProxy &operator=(T value)
    {
        m_value = std::move(value);
        return *this;
    }

    const QPersistentModelIndex &index() const noexcept { return m_index; }

private:
    QPersistentModelIndex m_index;
    std::optional<T> m_value;
};

} // namespace cute
