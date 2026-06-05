#include "CuteModel/Table.h"
#include "common/NonDefault.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

using cute::Table;
using cute_tests::NonDefault;

TEST(TableCtor, DefaultIsZeroByZero)
{
    Table<int> t;
    EXPECT_EQ(t.rows(), 0);
    EXPECT_EQ(t.columns(), 0);
    EXPECT_TRUE(t.empty());
}

TEST(TableCtor, DimensionsCtorFillsWithDefaultT)
{
    Table<int> t(2, 3);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.columns(), 3);
    EXPECT_FALSE(t.empty());
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(t.at(r, c), 0);
}

TEST(TableCtor, DimensionsCtorWithDefaultValueFills)
{
    Table<int> t(2, 3, 7);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(t.at(r, c), 7);
}

TEST(TableCtor, NegativeDimensionsClampToZero)
{
    Table<int> a(-1, 3);
    EXPECT_EQ(a.rows(), 0);
    EXPECT_EQ(a.columns(), 3);

    Table<int> b(2, -5);
    EXPECT_EQ(b.rows(), 2);
    EXPECT_EQ(b.columns(), 0);

    Table<int> c(-2, -3, 5);
    EXPECT_EQ(c.rows(), 0);
    EXPECT_EQ(c.columns(), 0);
    EXPECT_TRUE(c.empty());
}

TEST(TableCtor, NonDefaultConstructibleValueFill)
{
    Table<NonDefault> t(2, 2, NonDefault(9));
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0).value, 9);
    EXPECT_EQ(t.at(1, 1).value, 9);
}

TEST(TableAccess, AtConstReturnsCellValue)
{
    Table<int> t(1, 1, 42);
    const Table<int> &cref = t;
    EXPECT_EQ(cref.at(0, 0), 42);
}

TEST(TableAccess, AtAllowsMutation)
{
    Table<int> t(2, 2, 0);
    t.at(0, 1) = 5;
    t.at(1, 0) = 9;
    EXPECT_EQ(t.at(0, 0), 0);
    EXPECT_EQ(t.at(0, 1), 5);
    EXPECT_EQ(t.at(1, 0), 9);
    EXPECT_EQ(t.at(1, 1), 0);
}

TEST(TableRows, InsertRowsUniformFillsAllColumns)
{
    Table<int> t(0, 3);
    t.insert_rows(0, 2, 4);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.columns(), 3);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(t.at(r, c), 4);
}

TEST(TableRows, InsertRowsAtBeginningShiftsExisting)
{
    Table<int> t(1, 2, 9);
    t.insert_rows(0, 1, 1);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0), 1);
    EXPECT_EQ(t.at(0, 1), 1);
    EXPECT_EQ(t.at(1, 0), 9);
    EXPECT_EQ(t.at(1, 1), 9);
}

TEST(TableRows, InsertRowsAtEndAppends)
{
    Table<int> t(1, 2, 9);
    t.insert_rows(t.rows(), 1, 1);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0), 9);
    EXPECT_EQ(t.at(1, 0), 1);
}

TEST(TableRows, InsertRowsCountZeroIsNoOp)
{
    Table<int> t(2, 2, 3);
    t.insert_rows(1, 0, 7);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(1, 0), 3);
}

TEST(TableRows, InsertRowsPreShaped)
{
    Table<int> t(1, 2, 0);
    std::vector<std::vector<int>> data{{1, 2}, {3, 4}};
    t.insert_rows(0, std::move(data));
    EXPECT_EQ(t.rows(), 3);
    EXPECT_EQ(t.at(0, 0), 1);
    EXPECT_EQ(t.at(0, 1), 2);
    EXPECT_EQ(t.at(1, 0), 3);
    EXPECT_EQ(t.at(1, 1), 4);
    EXPECT_EQ(t.at(2, 0), 0);
}

TEST(TableRows, InsertRowsPreShapedEmptyIsNoOp)
{
    Table<int> t(2, 2, 5);
    t.insert_rows(0, std::vector<std::vector<int>>{});
    EXPECT_EQ(t.rows(), 2);
}

TEST(TableRows, EraseRowsInclusiveRange)
{
    Table<int> t(4, 1, 0);
    t.at(0, 0) = 10;
    t.at(1, 0) = 20;
    t.at(2, 0) = 30;
    t.at(3, 0) = 40;
    t.erase_rows(1, 2);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0), 10);
    EXPECT_EQ(t.at(1, 0), 40);
}

TEST(TableRows, EraseRowsSingleViaEqualBounds)
{
    Table<int> t(3, 1, 0);
    t.at(0, 0) = 1;
    t.at(1, 0) = 2;
    t.at(2, 0) = 3;
    t.erase_rows(1, 1);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0), 1);
    EXPECT_EQ(t.at(1, 0), 3);
}

TEST(TableRows, ResizeRowsGrowFillsNewRows)
{
    Table<int> t(1, 2, 5);
    t.resize_rows(3, 9);
    EXPECT_EQ(t.rows(), 3);
    EXPECT_EQ(t.at(0, 0), 5);
    EXPECT_EQ(t.at(1, 0), 9);
    EXPECT_EQ(t.at(1, 1), 9);
    EXPECT_EQ(t.at(2, 0), 9);
}

TEST(TableRows, ResizeRowsShrinkTruncates)
{
    Table<int> t(3, 1, 0);
    t.at(0, 0) = 10;
    t.at(1, 0) = 20;
    t.at(2, 0) = 30;
    t.resize_rows(1, 99);
    EXPECT_EQ(t.rows(), 1);
    EXPECT_EQ(t.at(0, 0), 10);
}

TEST(TableRows, ResizeRowsToSameIsNoOp)
{
    Table<int> t(2, 2, 4);
    t.resize_rows(2, 99);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0), 4);
}

TEST(TableColumns, InsertColumnsUniformFillsAllRows)
{
    Table<int> t(2, 0);
    t.insert_columns(0, 3, 7);
    EXPECT_EQ(t.columns(), 3);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(t.at(r, c), 7);
}

TEST(TableColumns, InsertColumnsAtBeginningShifts)
{
    Table<int> t(1, 1, 9);
    t.insert_columns(0, 1, 1);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0), 1);
    EXPECT_EQ(t.at(0, 1), 9);
}

TEST(TableColumns, InsertColumnsAtEndAppends)
{
    Table<int> t(1, 1, 9);
    t.insert_columns(t.columns(), 1, 1);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0), 9);
    EXPECT_EQ(t.at(0, 1), 1);
}

TEST(TableColumns, InsertColumnsCountZeroIsNoOp)
{
    Table<int> t(2, 2, 5);
    t.insert_columns(1, 0, 7);
    EXPECT_EQ(t.columns(), 2);
}

TEST(TableColumns, InsertColumnsPreShaped)
{
    Table<int> t(2, 1, 0);
    t.at(0, 0) = 1;
    t.at(1, 0) = 2;
    std::vector<std::vector<int>> cols{{10, 20}, {30, 40}};
    t.insert_columns(1, std::move(cols));
    EXPECT_EQ(t.columns(), 3);
    EXPECT_EQ(t.at(0, 0), 1);
    EXPECT_EQ(t.at(0, 1), 10);
    EXPECT_EQ(t.at(0, 2), 30);
    EXPECT_EQ(t.at(1, 0), 2);
    EXPECT_EQ(t.at(1, 1), 20);
    EXPECT_EQ(t.at(1, 2), 40);
}

TEST(TableColumns, EraseColumnsInclusiveRange)
{
    Table<int> t(1, 4, 0);
    t.at(0, 0) = 10;
    t.at(0, 1) = 20;
    t.at(0, 2) = 30;
    t.at(0, 3) = 40;
    t.erase_columns(1, 2);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0), 10);
    EXPECT_EQ(t.at(0, 1), 40);
}

TEST(TableColumns, ResizeColumnsGrowFillsExistingRows)
{
    Table<int> t(2, 1, 5);
    t.resize_columns(3, 9);
    EXPECT_EQ(t.columns(), 3);
    EXPECT_EQ(t.at(0, 0), 5);
    EXPECT_EQ(t.at(0, 1), 9);
    EXPECT_EQ(t.at(0, 2), 9);
    EXPECT_EQ(t.at(1, 1), 9);
}

TEST(TableColumns, ResizeColumnsShrinkTruncatesEachRow)
{
    Table<int> t(1, 3, 0);
    t.at(0, 0) = 10;
    t.at(0, 1) = 20;
    t.at(0, 2) = 30;
    t.resize_columns(2, 99);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0), 10);
    EXPECT_EQ(t.at(0, 1), 20);
}

TEST(TableColumns, ResizeColumnsToSameIsNoOp)
{
    Table<int> t(2, 2, 4);
    t.resize_columns(2, 99);
    EXPECT_EQ(t.columns(), 2);
    EXPECT_EQ(t.at(0, 0), 4);
}

TEST(TableReset, ReplacesContentAndDimensions)
{
    Table<int> t(2, 2, 0);
    std::vector<std::vector<int>> data{{1, 2, 3}, {4, 5, 6}};
    t.reset(std::move(data), 3);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.columns(), 3);
    EXPECT_EQ(t.at(0, 2), 3);
    EXPECT_EQ(t.at(1, 0), 4);
}

TEST(TableReset, EmptyResetProducesZeroByZero)
{
    Table<int> t(2, 2, 7);
    t.reset({}, 0);
    EXPECT_EQ(t.rows(), 0);
    EXPECT_EQ(t.columns(), 0);
    EXPECT_TRUE(t.empty());
}

TEST(TableDegenerate, ZeroRowsNonZeroColumns)
{
    Table<int> t(0, 4);
    EXPECT_EQ(t.rows(), 0);
    EXPECT_EQ(t.columns(), 4);
    EXPECT_TRUE(t.empty());
    t.insert_rows(0, 1, 7);
    EXPECT_EQ(t.rows(), 1);
    for (int c = 0; c < 4; ++c)
        EXPECT_EQ(t.at(0, c), 7);
}

TEST(TableDegenerate, NonZeroRowsZeroColumns)
{
    Table<int> t(3, 0);
    EXPECT_EQ(t.rows(), 3);
    EXPECT_EQ(t.columns(), 0);
    EXPECT_TRUE(t.empty());
    t.insert_columns(0, 2, 5);
    EXPECT_EQ(t.columns(), 2);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 2; ++c)
            EXPECT_EQ(t.at(r, c), 5);
}

TEST(TableNonDefault, InsertRowsUniformWithValue)
{
    Table<NonDefault> t(1, 2, NonDefault(1));
    t.insert_rows(t.rows(), 1, NonDefault(2));
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0).value, 1);
    EXPECT_EQ(t.at(1, 0).value, 2);
    EXPECT_EQ(t.at(1, 1).value, 2);
}

TEST(TableNonDefault, InsertRowsPreShaped)
{
    Table<NonDefault> t(0, 2, NonDefault(0));
    std::vector<std::vector<NonDefault>> rows;
    rows.push_back({NonDefault(1), NonDefault(2)});
    rows.push_back({NonDefault(3), NonDefault(4)});
    t.insert_rows(0, std::move(rows));
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 1).value, 2);
    EXPECT_EQ(t.at(1, 0).value, 3);
}

TEST(TableNonDefault, EraseAndResize)
{
    Table<NonDefault> t(3, 1, NonDefault(0));
    t.at(0, 0) = NonDefault(10);
    t.at(1, 0) = NonDefault(20);
    t.at(2, 0) = NonDefault(30);
    t.erase_rows(1, 1);
    EXPECT_EQ(t.rows(), 2);
    EXPECT_EQ(t.at(0, 0).value, 10);
    EXPECT_EQ(t.at(1, 0).value, 30);

    t.resize_rows(4, NonDefault(99));
    EXPECT_EQ(t.rows(), 4);
    EXPECT_EQ(t.at(2, 0).value, 99);
    EXPECT_EQ(t.at(3, 0).value, 99);
}
