#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>

#include <type_traits>
#include <vector>

namespace cute {

template <typename T, typename = void>
struct is_meta_object_compatible : std::false_type
{
};

template <typename T>
struct is_meta_object_compatible<T, std::void_t<decltype(T::staticMetaObject)>>
    : std::is_same<std::remove_cv_t<decltype(T::staticMetaObject)>, QMetaObject>
{
};

template <typename T>
constexpr bool is_meta_object_compatible_v =
    is_meta_object_compatible<T>::value;

template <typename T>
std::vector<QMetaProperty> metaProperties()
{
    static_assert(is_meta_object_compatible_v<T>,
                  "metaProperties<T>() requires a Q_OBJECT or Q_GADGET type");

    const QMetaObject &metaObject = T::staticMetaObject;

    std::vector<QMetaProperty> properties;
    properties.reserve(static_cast<std::size_t>(metaObject.propertyCount()));
    for (int i = 0; i < metaObject.propertyCount(); ++i)
        properties.push_back(metaObject.property(i));

    return properties;
}

}
