#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/qnamespace.h>

#include <memory>
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

template <typename T>
struct meta_element
{
    using type = T;
};

template <typename T>
struct meta_element<T *>
{
    using type = std::remove_cv_t<T>;
};

template <typename T>
struct meta_element<std::unique_ptr<T>>
{
    using type = std::remove_cv_t<T>;
};

template <typename T>
struct meta_element<std::shared_ptr<T>>
{
    using type = std::remove_cv_t<T>;
};

template <typename T>
using meta_element_t =
    typename meta_element<std::remove_cv_t<std::remove_reference_t<T>>>::type;

template <typename T>
constexpr bool has_meta_properties_v =
    is_meta_object_compatible_v<meta_element_t<T>>;

template <typename T>
struct is_shared_or_unique_ptr : std::false_type
{
};

template <typename T>
struct is_shared_or_unique_ptr<std::unique_ptr<T>> : std::true_type
{
};

template <typename T>
struct is_shared_or_unique_ptr<std::shared_ptr<T>> : std::true_type
{
};

constexpr int PropertyRoleBase = Qt::UserRole + 2;

template <typename T>
const meta_element_t<T> *metaElementPointer(const T &storage)
{
    using Stored = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_pointer_v<Stored>)
        return storage;
    else if constexpr (is_shared_or_unique_ptr<Stored>::value)
        return storage.get();
    else
        return std::addressof(storage);
}

template <typename T>
meta_element_t<T> *mutableMetaElementPointer(T &storage)
{
    using Stored = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_pointer_v<Stored>)
        return storage;
    else if constexpr (is_shared_or_unique_ptr<Stored>::value)
        return storage.get();
    else
        return std::addressof(storage);
}

template <typename T>
QVariant readMetaProperty(const T &storage, const QMetaProperty &property)
{
    using Element = meta_element_t<T>;
    const Element *pointer = metaElementPointer(storage);
    if (!pointer)
        return {};

    if constexpr (std::is_base_of_v<QObject, Element>)
        return property.read(pointer);
    else
        return property.readOnGadget(pointer);
}

template <typename T>
bool writeMetaProperty(T &storage, const QMetaProperty &property, const QVariant &value)
{
    using Element = meta_element_t<T>;
    Element *pointer = mutableMetaElementPointer(storage);
    if (!pointer)
        return false;

    if constexpr (std::is_base_of_v<QObject, Element>)
        return property.write(pointer, value);
    else
        return property.writeOnGadget(pointer, value);
}

template <typename T>
const std::vector<QMetaProperty> &metaPropertiesFor()
{
    static const std::vector<QMetaProperty> properties =
        metaProperties<meta_element_t<T>>();
    return properties;
}

template <typename T>
QVariant readMetaProperty(const T &storage, const char *name)
{
    const QMetaObject &metaObject = meta_element_t<T>::staticMetaObject;
    const int index = metaObject.indexOfProperty(name);
    if (index < 0)
        return {};
    return readMetaProperty(storage, metaObject.property(index));
}

template <typename T>
bool writeMetaProperty(T &storage, const char *name, const QVariant &value)
{
    const QMetaObject &metaObject = meta_element_t<T>::staticMetaObject;
    const int index = metaObject.indexOfProperty(name);
    if (index < 0)
        return false;
    return writeMetaProperty(storage, metaObject.property(index), value);
}

}
