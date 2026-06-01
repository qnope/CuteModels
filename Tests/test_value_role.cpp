#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using cute::is_compatible_with_value_role;
using cute::is_compatible_with_value_role_v;

namespace {

// Satisfies all three requirements: default constructible, destructible,
// copy constructible.
struct Compatible
{
    int value = 0;
};

// Not default constructible (no default ctor once a user ctor is declared).
struct NotDefaultConstructible
{
    explicit NotDefaultConstructible(int) {}
};

// Not copy constructible (copy explicitly deleted).
struct NotCopyConstructible
{
    NotCopyConstructible() = default;
    NotCopyConstructible(const NotCopyConstructible &) = delete;
};

// Not destructible (destructor deleted).
struct NotDestructible
{
    ~NotDestructible() = delete;
};

} // namespace

TEST(ValueRole, RoleConstantIsAfterUserRole)
{
    EXPECT_EQ(cute::ValueRole, Qt::UserRole + 1);
}

TEST(ValueRole, FundamentalTypesAreCompatible)
{
    EXPECT_TRUE(is_compatible_with_value_role_v<int>);
    EXPECT_TRUE(is_compatible_with_value_role_v<double>);
    EXPECT_TRUE(is_compatible_with_value_role_v<std::string>);
    EXPECT_TRUE((is_compatible_with_value_role_v<std::shared_ptr<int>>));
}

TEST(ValueRole, FullySatisfyingTypeIsCompatible)
{
    EXPECT_TRUE(is_compatible_with_value_role_v<Compatible>);
}

TEST(ValueRole, MissingDefaultConstructorIsNotCompatible)
{
    EXPECT_FALSE(is_compatible_with_value_role_v<NotDefaultConstructible>);
}

TEST(ValueRole, MissingCopyConstructorIsNotCompatible)
{
    EXPECT_FALSE(is_compatible_with_value_role_v<NotCopyConstructible>);
    // unique_ptr is the canonical move-only type — copy-construction fails.
    EXPECT_FALSE((is_compatible_with_value_role_v<std::unique_ptr<int>>));
}

TEST(ValueRole, NonDestructibleTypeIsNotCompatible)
{
    EXPECT_FALSE(is_compatible_with_value_role_v<NotDestructible>);
}

TEST(ValueRole, TraitTypeMatchesVariableTemplate)
{
    EXPECT_EQ(is_compatible_with_value_role<Compatible>::value,
              is_compatible_with_value_role_v<Compatible>);
    static_assert(is_compatible_with_value_role<int>::value,
                  "int must be compatible with ValueRole");
}
