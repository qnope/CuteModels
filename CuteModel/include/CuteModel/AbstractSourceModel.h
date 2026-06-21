#pragma once

#include "CuteModel/Exceptions.h"
#include "CuteModel/MetaObject.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueModelAccessor.h"
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
class AbstractSourceModel : public QAbstractItemModel, public ValueModelAccessor<T>
{
public:
    using QAbstractItemModel::QAbstractItemModel;

    virtual QString mimeTypeForValue() const
    {
        return QStringLiteral("application/AbstractSourceModel");
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
        const T &value = this->getStorageValue(index);
        const QHash<int, QByteArray> names = roleNames();
        for (QModelRoleData &roleData : roleDataSpan) {
            const int role = roleData.role();
            if constexpr (is_compatible_with_value_role_v<T>) {
                if (role == ValueRole) {
                    roleData.setData(QVariant::fromValue(value));
                    continue;
                }
            }
            if constexpr (has_meta_properties_v<T>) {
                if (const auto it = names.constFind(role);
                    role >= PropertyRoleBase && it != names.constEnd()) {
                    if (QVariant projected = readMetaProperty(value, it.value().constData());
                        projected.isValid()) {
                        roleData.data() = std::move(projected);
                    } else {
                        roleData.clearData();
                    }
                    continue;
                }
            }
            if (QVariant projected = data(value, index, role); projected.isValid()) {
                roleData.data() = std::move(projected);
            } else {
                roleData.clearData();
            }
        }
    }

    virtual QVariant data(const T &value, const QModelIndex &index, int role) const
    {
        return {};
    }

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

            this->setStorageValue(index, value.value<T>());
            return true;
        } else {
            return false;
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        static const QHash<int, QByteArray> names = [this]() {
            QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
            if constexpr (is_compatible_with_value_role_v<T>)
                names.insert(ValueRole, QByteArrayLiteral("value"));
            if constexpr (has_meta_properties_v<T>) {
                const std::vector<QMetaProperty> &properties = metaPropertiesFor<T>();
                for (std::size_t i = 0; i < properties.size(); ++i)
                    names.insert(PropertyRoleBase + static_cast<int>(i),
                                 QByteArray(properties[i].name()));
            }
            return names;
        }();
        return names;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const final
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return m_rootFlags;
        return flags(this->getStorageValue(index), index);
    }

    virtual Qt::ItemFlags flags(const T &value, const QModelIndex &index) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    Qt::ItemFlags rootFlags() const { return m_rootFlags; }

    void setRootFlags(Qt::ItemFlags flags) { m_rootFlags = flags; }

    QMimeData *mimeData(const QModelIndexList &indexes) const final
    {
        if constexpr (std::is_copy_constructible_v<T>) {
            std::vector<T> values;
            for (const QModelIndex &index : indexes) {
                if (!checkIndex(index, CheckIndexOption::IndexIsValid))
                    continue;
                values.push_back(this->getStorageValue(index));
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
        } else {
            return nullptr;
        }
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
            return canDropOnElement(this->getStorageValue(parent), parent, action, data);
        return canDropInsertion(row < 0 ? rowCount(parent) : row, column, parent,
                                action, data);
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) final
    {
        if (parent.isValid() && row == -1)
            return dropOnElement(this->getStorageValue(parent), parent, action, data);
        return dropInsertion(row < 0 ? rowCount(parent) : row, column, parent,
                             action, data);
    }

private:
    Qt::ItemFlags m_rootFlags = Qt::NoItemFlags;
};

}
