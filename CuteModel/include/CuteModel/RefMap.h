#pragma once

#include "CuteModel/MetaObject.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueModelAccessor.h"
#include "CuteModel/ValueRole.h"

#include <QQmlPropertyMap>
#include <QString>
#include <QVariant>

#include <memory>

namespace cute {

template <typename T>
class RefMap : public QQmlPropertyMap
{
    static_assert(has_meta_properties_v<T>,
                  "RefMap<T> requires a Q_OBJECT or Q_GADGET type");

public:
    using Ref = typename ValueModelAccessor<T>::Ref;

    explicit RefMap(std::unique_ptr<Ref> ref, QObject *parent = nullptr)
        : QQmlPropertyMap(parent)
        , m_ref(std::move(ref))
    {
        if (m_ref) {
            QObject::connect(m_ref.get(), &RefBase::valueChanged, this,
                             [this] { refresh(); });
            QObject::connect(m_ref.get(), &RefBase::underlyingValueDestroyed, this,
                             [this] { m_ref.reset(); });
        }
        refresh();
    }

protected:
    QVariant updateValue(const QString &key, const QVariant &input) override
    {
        if (!m_ref || !m_ref->index().isValid())
            return input;

        if constexpr (is_compatible_with_value_role_v<T>) {
            if (key == valueKey()) {
                if (input.canConvert<T>())
                    writeBack(input.value<T>());
                return input;
            }
        }

        T value = m_ref->getValue();
        writeMetaProperty(value, key.toUtf8().constData(), input);
        writeBack(value);
        return input;
    }

private:
    static QString valueKey() { return QStringLiteral("value"); }

    void writeBack(const T &value)
    {
        m_writingBack = true;
        m_ref->setValue(value);
        m_writingBack = false;
    }

    void refresh()
    {
        if (m_writingBack || !m_ref || !m_ref->index().isValid())
            return;

        const T &value = m_ref->getValue();
        for (const QMetaProperty &property : metaPropertiesFor<T>())
            insert(QString::fromUtf8(property.name()),
                   readMetaProperty(value, property));

        if constexpr (is_compatible_with_value_role_v<T>)
            insert(valueKey(), QVariant::fromValue(value));
    }

    std::unique_ptr<Ref> m_ref;
    bool m_writingBack = false;
};

}
