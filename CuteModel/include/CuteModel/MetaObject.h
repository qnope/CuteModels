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

// Unwraps the meta-object element type carried by T. Stored elements may be the
// gadget/object by value or behind an indirection (raw pointer, unique_ptr,
// shared_ptr); meta_element_t<T> resolves all of those to the underlying type.
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

// Property roles are numbered from PropertyRoleBase: the property at index i in
// metaPropertiesFor<T>() maps to role PropertyRoleBase + i. This keeps roleNames()
// and multiData() in agreement. PropertyRoleBase sits one past ValueRole
// (Qt::UserRole + 1).
constexpr int PropertyRoleBase = Qt::UserRole + 2;

// Returns a const pointer to the underlying meta element regardless of how T wraps
// it: the raw pointer itself for pointer forms, .get() for smart pointers, and the
// address of the value otherwise. May be null for the indirected forms.
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

// Reads a single meta-property off the stored element, choosing read() for
// QObject-derived types and readOnGadget() for gadgets. A null indirection yields
// an invalid QVariant.
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

// Cached property list for the meta element of T. The vector is built once per
// instantiation so roleNames()/multiData() never reallocate it.
template <typename T>
const std::vector<QMetaProperty> &metaPropertiesFor()
{
    static const std::vector<QMetaProperty> properties =
        metaProperties<meta_element_t<T>>();
    return properties;
}

}
