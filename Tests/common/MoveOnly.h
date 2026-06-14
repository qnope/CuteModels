#pragma once

#include <type_traits>

namespace cute_tests {

struct MoveOnly
{
    explicit MoveOnly(int v) : value(v) {}

    MoveOnly(MoveOnly &&) = default;
    MoveOnly &operator=(MoveOnly &&) = default;

    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;

    int value;
};

inline bool operator==(const MoveOnly &a, const MoveOnly &b)
{
    return a.value == b.value;
}

static_assert(!std::is_default_constructible_v<MoveOnly>);
static_assert(!std::is_copy_constructible_v<MoveOnly>);
static_assert(std::is_move_constructible_v<MoveOnly>);

}
