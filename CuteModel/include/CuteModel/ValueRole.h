#pragma once

#include <QtCore/qnamespace.h>

#include <type_traits>

namespace cute {

constexpr int ValueRole = Qt::UserRole + 1;

template <typename T>
struct is_compatible_with_value_role
    : std::conjunction<std::is_default_constructible<T>,
                       std::is_destructible<T>,
                       std::is_copy_constructible<T>>
{
};

template <typename T>
constexpr bool is_compatible_with_value_role_v =
    is_compatible_with_value_role<T>::value;

}
