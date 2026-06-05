#pragma once

#include <type_traits>

namespace cute_tests {

struct NonDefault
{
    explicit NonDefault(int v) : value(v) {}
    NonDefault(const NonDefault &) = default;
    NonDefault &operator=(const NonDefault &) = default;
    int value;
};

inline bool operator==(const NonDefault &a, const NonDefault &b)
{
    return a.value == b.value;
}

static_assert(!std::is_default_constructible_v<NonDefault>);

}
