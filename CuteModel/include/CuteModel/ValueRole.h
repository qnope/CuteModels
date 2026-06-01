#pragma once

#include <QtCore/qnamespace.h>

#include <type_traits>

namespace cute {

constexpr int ValueRole = Qt::UserRole + 1;

// Type trait describing whether a value type T can travel through the
// ValueRole storage hooks. A T flowing through ValueRole is default
// constructed (when the model materializes an empty slot), copied (when a
// value is read out of / written into storage) and destroyed (when the slot
// goes away), so it must satisfy all three operations.
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

} // namespace cute
