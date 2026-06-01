#pragma once

#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QVariant>

#include <exception>
#include <utility>

namespace cute {

template <typename T>
class BaseModel;

// Typed view over a single model item. Reads the item's ValueRole as a T.
// No Q_OBJECT here: the signals are inherited from RefBase (moc cannot process
// class templates).
//
// Refs are not constructed directly: BaseModel<T> is the sole factory
// (BaseModel::getRef), so the constructor is protected and BaseModel is a
// friend.
template <typename T>
class Ref : public RefBase
{
public:
    T getValue() const
    {
        if (!m_index.isValid())
            std::terminate();

        QVariant value = m_index.data(ValueRole);
        if (!value.isValid())
            std::terminate();

        return value.template value<T>();
    }

    // Writes `value` back into the model item through its ValueRole. Returns
    // the model's setData result (false when the write is rejected). An invalid
    // index — or one whose model has gone away — is a programming error and
    // terminates, mirroring getValue.
    bool setValue(const T &value)
    {
        if (!m_index.isValid())
            std::terminate();

        auto *model = const_cast<QAbstractItemModel *>(m_index.model());
        if (!model)
            std::terminate();

        return model->setData(m_index, QVariant::fromValue(value), ValueRole);
    }

protected:
    explicit Ref(QPersistentModelIndex index) : RefBase(std::move(index)) {}

    template <typename U>
    friend class BaseModel;
};

} // namespace cute
