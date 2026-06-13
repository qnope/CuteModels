#pragma once

#include "CuteModel/Exceptions.h"
#include "CuteModel/RefBase.h"

#include <QModelIndex>
#include <QPersistentModelIndex>

#include <memory>
#include <type_traits>
#include <utility>

namespace cute {

template <typename T>
class ValueModelAccessor
{
public:
    virtual ~ValueModelAccessor() = default;

    class Ref : public RefBase
    {
    public:
        const T &getValue() const
        {
            if (!m_index.isValid() || !m_model)
                throw NullObjectException(
                    "ValueModelAccessor::Ref::getValue called on an invalidated reference");

            return m_model->getStorageValue(m_index);
        }

        void setValue(const T &value)
        {
            if (!m_index.isValid() || !m_model)
                throw NullObjectException(
                    "ValueModelAccessor::Ref::setValue called on an invalidated reference");

            m_model->setStorageValue(m_index, value);
        }

    protected:
        Ref(ValueModelAccessor *model, QPersistentModelIndex index)
            : RefBase(std::move(index))
            , m_model(model)
        {}

        ValueModelAccessor *m_model;

        friend class ValueModelAccessor;
    };

    // The accessor/index pair a Ref should bind to. For a plain source this is the
    // accessor itself; proxies resolve it down to the underlying source (see below).
    struct ResolvedSource
    {
        ValueModelAccessor *accessor = nullptr;
        QModelIndex index;
    };

    // Customization point that walks a proxy chain back to its source. The default,
    // used by every AbstractSourceModel, is the identity: an accessor *is* its own
    // source. A proxy overrides this to map the index one level down and recurse,
    // so the resolution terminates at the first non-proxy accessor without any RTTI.
    virtual ResolvedSource resolveSource(const QModelIndex &index)
    {
        return {this, index};
    }

    template <typename R = Ref>
    std::unique_ptr<R> getRef(const QModelIndex &index)
    {
        static_assert(std::is_base_of_v<Ref, R>,
                      "getRef<R> requires R to derive from ValueModelAccessor<T>::Ref");

        if (!index.isValid())
            return nullptr;

        // A Ref built from a proxy index follows the proxy back to its source so that
        // it tracks the owning AbstractSourceModel directly, surviving (and observing)
        // changes even when the proxy would filter the row out.
        const ResolvedSource resolved = resolveSource(index);
        if (!resolved.accessor || !resolved.index.isValid())
            return nullptr;

        return std::unique_ptr<R>(
            new R(resolved.accessor, QPersistentModelIndex(resolved.index)));
    }

    virtual const T &getStorageValue(const QModelIndex &index) const = 0;

    virtual void setStorageValue(const QModelIndex &index, T value) = 0;
};

}
