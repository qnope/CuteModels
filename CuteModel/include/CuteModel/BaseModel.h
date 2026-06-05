#pragma once

#include "CuteModel/Exceptions.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QVariant>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

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
                throw NullObjectException(
                    "BaseModel::Ref::getValue called on an invalidated reference");

            return m_model->getStorageValue(m_index);
        }

        void setValue(const T &value)
        {
            if (!m_index.isValid() || !m_model)
                throw NullObjectException(
                    "BaseModel::Ref::setValue called on an invalidated reference");

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

    virtual QString mimeTypeForValue() const
    {
        return QStringLiteral("application/BaseModel");
    }

    virtual void encodeMimeData(QDataStream &stream, const std::vector<T> &values) const {}

    virtual std::vector<T> decodeMimeData(QDataStream &stream) const { return {}; }

    virtual bool canDropOnElement(const T &element, const QModelIndex &index,
                                  Qt::DropAction action, const QMimeData *data) const
    {
        return false;
    }

    virtual bool dropOnElement(const T &element, const QModelIndex &index,
                               Qt::DropAction action, const QMimeData *data)
    {
        return false;
    }

    virtual bool canDropInsertion(int row, int column, const QModelIndex &parent,
                                  Qt::DropAction action, const QMimeData *data) const
    {
        return false;
    }

    virtual bool dropInsertion(int row, int column, const QModelIndex &parent,
                               Qt::DropAction action, const QMimeData *data)
    {
        return false;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final
    {
        QModelRoleData roleData(role);
        multiData(index, roleData);
        return roleData.data();
    }

    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const final
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        const T &value = getStorageValue(index);
        for (QModelRoleData &roleData : roleDataSpan) {
            if constexpr (is_compatible_with_value_role_v<T>) {
                if (roleData.role() == ValueRole) {
                    roleData.setData(QVariant::fromValue(value));
                    continue;
                }
            }
            if (QVariant projected = data(value, index, roleData.role());
                projected.isValid()) {
                roleData.data() = std::move(projected);
            } else {
                roleData.clearData();
            }
        }
    }

    virtual QVariant data(const T &value, const QModelIndex &index, int role) const = 0;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = ValueRole) override
    {
        if constexpr (is_compatible_with_value_role_v<T>) {
            if (role != ValueRole)
                return false;
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

    Qt::ItemFlags flags(const QModelIndex &index) const final
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return m_rootFlags;
        return flags(getStorageValue(index), index);
    }

    virtual Qt::ItemFlags flags(const T &value, const QModelIndex &index) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    Qt::ItemFlags rootFlags() const { return m_rootFlags; }

    void setRootFlags(Qt::ItemFlags flags) { m_rootFlags = flags; }

    QMimeData *mimeData(const QModelIndexList &indexes) const final
    {
        std::vector<T> values;
        for (const QModelIndex &index : indexes) {
            if (!checkIndex(index, CheckIndexOption::IndexIsValid))
                continue;
            values.push_back(getStorageValue(index));
        }
        if (values.empty())
            return nullptr;

        QByteArray encoded;
        QDataStream stream(&encoded, QIODevice::WriteOnly);
        encodeMimeData(stream, values);
        if (encoded.isEmpty())
            return nullptr;

        auto *mimeData = new QMimeData;
        mimeData->setData(mimeTypeForValue(), encoded);
        return mimeData;
    }

    std::vector<T> valuesFromMimeData(const QMimeData *mimeData) const
    {
        if (!mimeData)
            return {};

        QByteArray encoded = mimeData->data(mimeTypeForValue());
        if (encoded.isEmpty())
            return {};

        QDataStream stream(&encoded, QIODevice::ReadOnly);
        return decodeMimeData(stream);
    }

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                         int row, int column, const QModelIndex &parent) const final
    {
        if (parent.isValid() && row == -1)
            return canDropOnElement(getStorageValue(parent), parent, action, data);
        return canDropInsertion(row < 0 ? rowCount(parent) : row, column, parent,
                                action, data);
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) final
    {
        if (parent.isValid() && row == -1)
            return dropOnElement(getStorageValue(parent), parent, action, data);
        return dropInsertion(row < 0 ? rowCount(parent) : row, column, parent,
                             action, data);
    }

protected:
    virtual const T &getStorageValue(const QModelIndex &index) const = 0;

    virtual void setStorageValue(const QModelIndex &index, T value) = 0;

private:
    Qt::ItemFlags m_rootFlags = Qt::NoItemFlags;
};

}
