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

// The trait is a pure compile-time predicate, so the whole contract is
// expressed with static_assert and verified when this translation unit is
// built — no runtime assertions are needed.

// The role constant sits just past Qt::UserRole.
static_assert(cute::ValueRole == Qt::UserRole + 1,
              "ValueRole must be the first user role");

// Fundamental and standard types that travel through ValueRole are compatible.
static_assert(is_compatible_with_value_role_v<int>);
static_assert(is_compatible_with_value_role_v<double>);
static_assert(is_compatible_with_value_role_v<std::string>);
static_assert(is_compatible_with_value_role_v<std::shared_ptr<int>>);

// A type satisfying every requirement is compatible.
static_assert(is_compatible_with_value_role_v<Compatible>);

// Each missing requirement makes the type incompatible.
static_assert(!is_compatible_with_value_role_v<NotDefaultConstructible>);
static_assert(!is_compatible_with_value_role_v<NotCopyConstructible>);
static_assert(!is_compatible_with_value_role_v<NotDestructible>);
// unique_ptr is the canonical move-only type — copy-construction fails.
static_assert(!is_compatible_with_value_role_v<std::unique_ptr<int>>);

// The variable template mirrors the trait type's ::value member.
static_assert(is_compatible_with_value_role<Compatible>::value ==
              is_compatible_with_value_role_v<Compatible>);

} // namespace

// The contract above is enforced at compile time; this case simply gives the
// test runner a discoverable entry confirming the translation unit built.
TEST(ValueRole, CompileTimeContractHolds)
{
    SUCCEED();
}
