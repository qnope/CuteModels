#pragma once

#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QVariant>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace cute {

template <typename T>
class BaseModel : public QAbstractItemModel
{
public:
    using QAbstractItemModel::QAbstractItemModel;

    class Ref : public RefBase
    {
    public:
        const T &getValue() const
        {
            if (!m_index.isValid() || !m_model)
                std::terminate();

            return m_model->getStorageValue(m_index);
        }

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

    template <typename R = Ref>
    std::unique_ptr<R> getRef(const QModelIndex &index)
    {
        static_assert(std::is_base_of_v<Ref, R>,
                      "getRef<R> requires R to derive from BaseModel<T>::Ref");

        if (!index.isValid())
            return nullptr;

        return std::unique_ptr<R>(new R(this, QPersistentModelIndex(index)));
    }

    virtual QMimeData *mimeDataForValue(const T &value) const { return nullptr; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final
    {
        QModelRoleData roleData(role);
        multiData(index, roleData);
        return roleData.data();
    }

    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        if constexpr (is_compatible_with_value_role_v<T>) {
            for (QModelRoleData &roleData : roleDataSpan) {
                if (roleData.role() == ValueRole) {
                    roleData.setData(QVariant::fromValue(getStorageValue(index)));
                    return;
                }
            }
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = ValueRole) override
    {
        if constexpr (is_compatible_with_value_role_v<T>) {
            if (role != ValueRole)
                std::terminate();
            if (!checkIndex(index, CheckIndexOption::IndexIsValid))
                return false;
            if (!value.canConvert<T>())
                return false;

            setStorageValue(index, value.value<T>());
            return true;
        } else {
            return false;
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
        if constexpr (is_compatible_with_value_role_v<T>)
            names.insert(ValueRole, QByteArrayLiteral("value"));
        return names;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const final
    {
        if (indexes.size() != 1)
            return nullptr;
        const QModelIndex &index = indexes.front();
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return nullptr;
        return mimeDataForValue(getStorageValue(index));
    }

protected:
    virtual const T &getStorageValue(const QModelIndex &index) const = 0;

    virtual void setStorageValue(const QModelIndex &index, T value) = 0;
};

}
