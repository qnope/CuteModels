#pragma once

#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <QVariant>

#include <exception>

namespace cute {

// Typed view over a single model item. Reads the item's ValueRole as a T.
// No Q_OBJECT here: the signals are inherited from RefBase (moc cannot process
// class templates).
template <typename T>
class Ref : public RefBase
{
public:
    using RefBase::RefBase;

    T getValue() const
    {
        if (!m_index.isValid())
            std::terminate();

        QVariant value = m_index.data(ValueRole);
        if (!value.isValid())
            std::terminate();

        return value.template value<T>();
    }
};

} // namespace cute
