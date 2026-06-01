#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using cute::is_compatible_with_value_role;
using cute::is_compatible_with_value_role_v;

namespace {

struct Compatible
{
    int value = 0;
};

struct NotDefaultConstructible
{
    explicit NotDefaultConstructible(int) {}
};

struct NotCopyConstructible
{
    NotCopyConstructible() = default;
    NotCopyConstructible(const NotCopyConstructible &) = delete;
};

struct NotDestructible
{
    ~NotDestructible() = delete;
};

static_assert(cute::ValueRole == Qt::UserRole + 1,
              "ValueRole must be the first user role");

static_assert(is_compatible_with_value_role_v<int>);
static_assert(is_compatible_with_value_role_v<double>);
static_assert(is_compatible_with_value_role_v<std::string>);
static_assert(is_compatible_with_value_role_v<std::shared_ptr<int>>);

static_assert(is_compatible_with_value_role_v<Compatible>);

static_assert(!is_compatible_with_value_role_v<NotDefaultConstructible>);
static_assert(!is_compatible_with_value_role_v<NotCopyConstructible>);
static_assert(!is_compatible_with_value_role_v<NotDestructible>);
static_assert(!is_compatible_with_value_role_v<std::unique_ptr<int>>);

static_assert(is_compatible_with_value_role<Compatible>::value ==
              is_compatible_with_value_role_v<Compatible>);

}

TEST(ValueRole, CompileTimeContractHolds)
{
    SUCCEED();
}
