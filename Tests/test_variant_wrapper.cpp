#include "CuteModel/VariantWrapper.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using cute::VariantWrapper;

namespace {

// A type deliberately without Q_DECLARE_METATYPE support, with non-trivial
// construction/destruction to exercise lifetime management.
struct NonTrivial
{
    std::string name;
    std::vector<int> data;
};

} // namespace

TEST(VariantWrapper, StoresAndRetrievesValue)
{
    VariantWrapper<NonTrivial> wrapper(NonTrivial{"hello", {1, 2, 3}});

    EXPECT_EQ(wrapper.get().name, "hello");
    EXPECT_EQ(wrapper->data, (std::vector<int>{1, 2, 3}));
}

TEST(VariantWrapper, CopyIsIndependent)
{
    VariantWrapper<NonTrivial> original(NonTrivial{"a", {1}});
    VariantWrapper<NonTrivial> copy(original);

    copy->name = "b";
    copy->data.push_back(2);

    EXPECT_EQ(original->name, "a");
    EXPECT_EQ(original->data, (std::vector<int>{1}));
    EXPECT_EQ(copy->name, "b");
    EXPECT_EQ(copy->data, (std::vector<int>{1, 2}));
}

TEST(VariantWrapper, MovePreservesContents)
{
    VariantWrapper<std::string> source(std::string("movable"));
    VariantWrapper<std::string> moved(std::move(source));

    EXPECT_EQ(moved.get(), "movable");
}

TEST(VariantWrapper, CopyAssignmentReplacesContents)
{
    VariantWrapper<int> a(1);
    VariantWrapper<int> b(2);

    a = b;

    EXPECT_EQ(a.get(), 2);
}
