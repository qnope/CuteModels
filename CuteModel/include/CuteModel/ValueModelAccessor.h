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
    using value_type = T;

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

    template <typename R = Ref>
    std::unique_ptr<R> getRef(const QModelIndex &index)
    {
        static_assert(std::is_base_of_v<Ref, R>,
                      "getRef<R> requires R to derive from ValueModelAccessor<T>::Ref");

        if (!index.isValid())
            return nullptr;

        return std::unique_ptr<R>(new R(this, QPersistentModelIndex(index)));
    }

    virtual const T &getStorageValue(const QModelIndex &index) const = 0;

    virtual void setStorageValue(const QModelIndex &index, T value) = 0;
};

}
