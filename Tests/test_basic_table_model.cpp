#include "CuteModel/BasicTableModel.h"
#include "CuteModel/ValueRole.h"
#include "common/NonDefault.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QModelIndex>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <vector>

using cute::BasicTableModel;
using cute::ValueRole;

namespace {

class IntTableModel : public BasicTableModel<int>
{
public:
    using BasicTableModel<int>::BasicTableModel;
    using BasicTableModel<int>::data;

    QVariant data(const int &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return value;
        return {};
    }
};

struct Pod
{
    int id = 0;
    QString label;
};

bool operator==(const Pod &a, const Pod &b)
{
    return a.id == b.id && a.label == b.label;
}

class PodTableModel : public BasicTableModel<Pod>
{
public:
    using BasicTableModel<Pod>::BasicTableModel;
    using BasicTableModel<Pod>::data;

    QVariant data(const Pod &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value.label;
        if (role == Qt::EditRole)
            return value.id;
        return {};
    }
};

class IntTableModelTest : public ::testing::Test
{
protected:
    IntTableModel model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

class PodTableModelTest : public ::testing::Test
{
protected:
    PodTableModel model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

}

TEST(BasicTableModelCtor, DefaultIsZeroByZero)
{
    IntTableModel m;
    QAbstractItemModelTester guard{&m,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_EQ(m.rowCount(), 0);
    EXPECT_EQ(m.columnCount(), 0);
    EXPECT_EQ(m.rows(), 0);
    EXPECT_EQ(m.columns(), 0);
    EXPECT_TRUE(m.empty());
}

TEST(BasicTableModelCtor, DimensionsCtorFillsWithDefaultT)
{
    IntTableModel m(2, 3);
    QAbstractItemModelTester guard{&m,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_EQ(m.rowCount(), 2);
    EXPECT_EQ(m.columnCount(), 3);
    EXPECT_FALSE(m.empty());
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(m.index(r, c).data(ValueRole).toInt(), 0);
}

TEST(BasicTableModelCtor, DimensionsCtorWithDefaultValueFills)
{
    IntTableModel m(2, 3, 7);
    QAbstractItemModelTester guard{&m,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_EQ(m.index(r, c).data(ValueRole).toInt(), 7);
}

TEST(BasicTableModelCtor, NegativeDimensionsClampToZero)
{
    IntTableModel m(-3, -5, 9);
    QAbstractItemModelTester guard{&m,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_EQ(m.rowCount(), 0);
    EXPECT_EQ(m.columnCount(), 0);
}

TEST_F(IntTableModelTest, RowCountColumnCountWithValidParentReturnsZero)
{
    model.resize_rows(2);
    model.resize_columns(2);
    const QModelIndex cell = model.index(0, 0);
    ASSERT_TRUE(cell.isValid());
    EXPECT_EQ(model.rowCount(cell), 0);
    EXPECT_EQ(model.columnCount(cell), 0);
}

TEST_F(IntTableModelTest, IndexOutOfBoundsReturnsInvalid)
{
    model.resize_rows(2);
    model.resize_columns(2);
    EXPECT_FALSE(model.index(-1, 0).isValid());
    EXPECT_FALSE(model.index(0, -1).isValid());
    EXPECT_FALSE(model.index(2, 0).isValid());
    EXPECT_FALSE(model.index(0, 2).isValid());
}

TEST_F(IntTableModelTest, IndexWithValidParentReturnsInvalid)
{
    model.resize_rows(2);
    model.resize_columns(2);
    const QModelIndex cell = model.index(0, 0);
    EXPECT_FALSE(model.index(0, 0, cell).isValid());
}

TEST_F(IntTableModelTest, ParentAlwaysInvalid)
{
    model.resize_rows(1);
    model.resize_columns(1);
    EXPECT_FALSE(model.parent(model.index(0, 0)).isValid());
}

TEST_F(IntTableModelTest, SetDataEmitsCellOnlyDataChanged)
{
    model.resize_rows(2);
    model.resize_columns(3);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_TRUE(model.setData(model.index(1, 2), 42, ValueRole));

    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(1, 2));
    EXPECT_EQ(args.at(1).value<QModelIndex>(), model.index(1, 2));
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 42);
}

TEST_F(IntTableModelTest, SetDataDoesNotTouchSiblingCells)
{
    model.resize_rows(2);
    model.resize_columns(2);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 1), 2, ValueRole));
    EXPECT_TRUE(model.setData(model.index(1, 0), 3, ValueRole));
    EXPECT_TRUE(model.setData(model.index(1, 1), 4, ValueRole));

    EXPECT_TRUE(model.setData(model.index(0, 1), 99, ValueRole));

    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 99);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, DisplayRoleDelegatesToVirtualData)
{
    model.resize_rows(1);
    model.resize_columns(1);
    EXPECT_TRUE(model.setData(model.index(0, 0), 5, ValueRole));
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toInt(), 5);
    EXPECT_FALSE(model.index(0, 0).data(9999).isValid());
}

TEST_F(PodTableModelTest, ValueRoleRoundTripPerCell)
{
    model.resize_rows(1);
    model.resize_columns(2);

    const Pod a{1, QStringLiteral("alpha")};
    const Pod b{2, QStringLiteral("beta")};
    EXPECT_TRUE(model.setData(model.index(0, 0), QVariant::fromValue(a), ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 1), QVariant::fromValue(b), ValueRole));

    EXPECT_TRUE(model.index(0, 0).data(ValueRole).value<Pod>() == a);
    EXPECT_TRUE(model.index(0, 1).data(ValueRole).value<Pod>() == b);
}

TEST_F(PodTableModelTest, MultiDataFillsHandledRoles)
{
    model.resize_rows(1);
    model.resize_columns(1);
    const Pod p{7, QStringLiteral("seven")};
    EXPECT_TRUE(model.setData(model.index(0, 0), QVariant::fromValue(p), ValueRole));

    QModelRoleData roles[] = {
        QModelRoleData(ValueRole),
        QModelRoleData(Qt::DisplayRole),
        QModelRoleData(Qt::EditRole),
        QModelRoleData(9999),
    };
    QModelRoleDataSpan span(roles, 4);
    model.multiData(model.index(0, 0), span);

    EXPECT_TRUE(roles[0].data().value<Pod>() == p);
    EXPECT_EQ(roles[1].data().toString(), QStringLiteral("seven"));
    EXPECT_EQ(roles[2].data().toInt(), 7);
    EXPECT_FALSE(roles[3].data().isValid());
}

TEST_F(IntTableModelTest, PushBackRowEmitsRowsInserted)
{
    model.resize_columns(2);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.push_back_row(3);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, InsertRowShiftsExisting)
{
    model.resize_columns(1);
    model.push_back_row(1);
    model.push_back_row(3);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_row(1, 2);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, InsertRowsAppendsSingleSignal)
{
    model.resize_columns(1);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_rows(0, 3, 5);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.rowCount(), 3);
    for (int r = 0; r < 3; ++r)
        EXPECT_EQ(model.index(r, 0).data(ValueRole).toInt(), 5);
}

TEST_F(IntTableModelTest, InsertRowsOutOfRangeNoOp)
{
    model.resize_columns(1);
    model.push_back_row(0);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_rows(-1, 1);
    model.insert_rows(2, 1);
    model.insert_rows(0, 0);
    model.insert_rows(0, -3);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, InsertRangeRowSingleFillsEachColumn)
{
    model.resize_columns(3);
    model.push_back_row(0);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range_row(0, std::vector<int>{1, 2, 3});

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 0);
}

TEST_F(IntTableModelTest, AppendRangeRowSingleAddsAtEnd)
{
    model.resize_columns(2);
    model.push_back_row(9);

    model.append_range_row(std::vector<int>{1, 2});

    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 2);
}

TEST_F(IntTableModelTest, AppendRangeRowSingleEstablishesColumnsOnEmptyModel)
{
    QSignalSpy columnsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    model.append_range_row(std::vector<int>{4, 5, 6});

    ASSERT_EQ(columnsSpy.count(), 1);
    EXPECT_EQ(columnsSpy.at(0).at(1).toInt(), 0);
    EXPECT_EQ(columnsSpy.at(0).at(2).toInt(), 2);
    ASSERT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 4);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 6);
}

TEST_F(IntTableModelTest, InsertRangeRowMultipleEmitsSingleSignal)
{
    model.resize_columns(2);
    model.push_back_row(0);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range_row(0, std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 4);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 0);
}

TEST_F(IntTableModelTest, AppendRangeRowMultipleAddsAtEnd)
{
    model.resize_columns(2);

    model.append_range_row(std::vector<std::vector<int>>{{1, 2}, {3, 4}, {5, 6}});

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 5);
    EXPECT_EQ(model.index(2, 1).data(ValueRole).toInt(), 6);
}

TEST_F(IntTableModelTest, InsertRangeRowOutOfRangeNoOp)
{
    model.resize_columns(2);
    model.push_back_row(0);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range_row(-1, std::vector<int>{1, 2});
    model.insert_range_row(2, std::vector<int>{1, 2});
    model.insert_range_row(0, std::vector<std::vector<int>>{});

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, InsertRangeColumnSingleFillsEachRow)
{
    model.resize_rows(3);
    model.resize_columns(1);
    for (int r = 0; r < 3; ++r)
        EXPECT_TRUE(model.setData(model.index(r, 0), 9, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_range_column(0, std::vector<int>{1, 2, 3});

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 9);
}

TEST_F(IntTableModelTest, AppendRangeColumnSingleAddsAtEnd)
{
    model.resize_rows(2);
    model.resize_columns(1);

    model.append_range_column(std::vector<int>{7, 8});

    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 8);
}

TEST_F(IntTableModelTest, InsertRangeColumnMultipleShiftsExistingValues)
{
    model.resize_rows(2);
    model.resize_columns(1);
    EXPECT_TRUE(model.setData(model.index(0, 0), 100, ValueRole));
    EXPECT_TRUE(model.setData(model.index(1, 0), 200, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_range_column(0, std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 100);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 4);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 200);
}

TEST_F(IntTableModelTest, AppendRangeColumnMultipleAddsAtEnd)
{
    model.resize_rows(2);
    model.resize_columns(1);

    model.append_range_column(std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, InsertRangeColumnOutOfRangeNoOp)
{
    model.resize_rows(2);
    model.resize_columns(1);

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_range_column(-1, std::vector<int>{1, 2});
    model.insert_range_column(2, std::vector<int>{1, 2});
    model.insert_range_column(0, std::vector<std::vector<int>>{});

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.columnCount(), 1);
}

TEST_F(IntTableModelTest, EraseRowEmitsRowsRemoved)
{
    model.resize_columns(1);
    model.push_back_row(1);
    model.push_back_row(2);
    model.push_back_row(3);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    model.erase_row(1);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, EraseRowsRangeEmitsSingleSignal)
{
    model.resize_columns(1);
    model.insert_rows(0, 5, 0);
    for (int r = 0; r < 5; ++r)
        model.setData(model.index(r, 0), r, ValueRole);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    model.erase_rows(1, 3);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, EraseRowsOutOfRangeNoOp)
{
    model.resize_columns(1);
    model.push_back_row(0);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    model.erase_rows(-1, 0);
    model.erase_rows(2, 2);
    model.erase_rows(0, -1);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, ResizeRowsGrowAndShrink)
{
    model.resize_columns(1);
    model.push_back_row(1);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.resize_rows(4, 9);

    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(insertedSpy.at(0).at(1).toInt(), 1);
    EXPECT_EQ(insertedSpy.at(0).at(2).toInt(), 3);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.index(3, 0).data(ValueRole).toInt(), 9);

    model.resize_rows(2);
    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(removedSpy.at(0).at(1).toInt(), 2);
    EXPECT_EQ(removedSpy.at(0).at(2).toInt(), 3);
    EXPECT_EQ(model.rowCount(), 2);
}

TEST_F(IntTableModelTest, PushBackColumnEmitsColumnsInserted)
{
    model.resize_rows(2);

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.push_back_column(7);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
    EXPECT_EQ(model.columnCount(), 1);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 7);
}

TEST_F(IntTableModelTest, InsertColumnShiftsExistingValues)
{
    model.resize_rows(1);
    model.resize_columns(2);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 1), 3, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_column(1, 2);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, InsertColumnsAtEndAppends)
{
    model.resize_rows(1);
    model.resize_columns(1);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_columns(model.columnCount(), 2, 5);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 5);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 5);
}

TEST_F(IntTableModelTest, InsertColumnsOutOfRangeNoOp)
{
    model.resize_rows(1);
    model.resize_columns(1);

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    model.insert_columns(-1, 1);
    model.insert_columns(2, 1);
    model.insert_columns(0, 0);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.columnCount(), 1);
}

TEST_F(IntTableModelTest, EraseColumnEmitsColumnsRemoved)
{
    model.resize_rows(1);
    model.resize_columns(3);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 1), 2, ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 2), 3, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsRemoved);

    model.erase_column(1);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, EraseColumnsRangeEmitsSingleSignal)
{
    model.resize_rows(1);
    model.resize_columns(5);
    for (int c = 0; c < 5; ++c)
        EXPECT_TRUE(model.setData(model.index(0, c), c, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsRemoved);

    model.erase_columns(1, 3);

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, ResizeColumnsGrowAndShrink)
{
    model.resize_rows(1);
    model.resize_columns(1);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy removedSpy(&model, &QAbstractItemModel::columnsRemoved);

    model.resize_columns(4, 9);
    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(insertedSpy.at(0).at(1).toInt(), 1);
    EXPECT_EQ(insertedSpy.at(0).at(2).toInt(), 3);
    EXPECT_EQ(model.columnCount(), 4);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 3).data(ValueRole).toInt(), 9);

    model.resize_columns(2);
    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(removedSpy.at(0).at(1).toInt(), 2);
    EXPECT_EQ(removedSpy.at(0).at(2).toInt(), 3);
    EXPECT_EQ(model.columnCount(), 2);
}

TEST_F(IntTableModelTest, ClearEmitsModelResetAndProducesZeroByZero)
{
    model.resize_rows(2);
    model.resize_columns(3);

    QSignalSpy spy(&model, &QAbstractItemModel::modelReset);

    model.clear();

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 0);
    EXPECT_TRUE(model.empty());
}

TEST_F(IntTableModelTest, ResetSetsDimensionsAndContent)
{
    QSignalSpy spy(&model, &QAbstractItemModel::modelReset);

    model.reset({{1, 2, 3}, {4, 5, 6}});

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 4);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 6);
}

TEST_F(IntTableModelTest, ResetWithEmptyVectorIsZeroByZero)
{
    model.resize_rows(2);
    model.resize_columns(2);

    model.reset({});

    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 0);
}

TEST_F(IntTableModelTest, GetMutableEmitsCellDataChangedOnCommit)
{
    model.resize_rows(2);
    model.resize_columns(2);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.getMutable(1, 1) = 42;

    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 42);
    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(1, 1));
    EXPECT_EQ(args.at(1).value<QModelIndex>(), model.index(1, 1));
}

TEST_F(IntTableModelTest, GetMutableDefersDataChangedUntilDestruction)
{
    model.resize_rows(1);
    model.resize_columns(1);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto proxy = model.getMutable(0, 0);
        *proxy = 5;
        EXPECT_EQ(changedSpy.count(), 0);
    }
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(IntTableModelTest, AtReturnsCellValue)
{
    model.resize_rows(2);
    model.resize_columns(2);
    EXPECT_TRUE(model.setData(model.index(1, 0), 11, ValueRole));

    EXPECT_EQ(*model.at(1, 0), 11);
}

TEST_F(IntTableModelTest, FlagsDefaultPerCellAndRootConfigurable)
{
    model.resize_rows(1);
    model.resize_columns(1);

    EXPECT_EQ(model.flags(model.index(0, 0)),
              Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    EXPECT_EQ(model.flags(QModelIndex()), Qt::NoItemFlags);

    model.setRootFlags(Qt::ItemIsDropEnabled);
    EXPECT_EQ(model.flags(QModelIndex()), Qt::ItemIsDropEnabled);
}

TEST_F(IntTableModelTest, DegenerateNRowsZeroColumnsIsValid)
{
    model.resize_rows(3);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.columnCount(), 0);
    EXPECT_TRUE(model.empty());
    EXPECT_FALSE(model.index(0, 0).isValid());
}

TEST_F(IntTableModelTest, DegenerateZeroRowsNColumnsIsValid)
{
    model.resize_columns(3);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_TRUE(model.empty());
    EXPECT_FALSE(model.index(0, 0).isValid());

    model.push_back_row(8);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 8);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 8);
}

TEST_F(IntTableModelTest, InsertRowsOverrideInsertsAndEmitsSignal)
{
    model.resize_columns(2);
    model.push_back_row(1);
    model.push_back_row(3);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insertRows(1, 2));

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(3, 0).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, InsertRowsOverrideAppendsAtEnd)
{
    model.resize_columns(1);
    model.push_back_row(1);

    EXPECT_TRUE(model.insertRows(model.rowCount(), 2));
    EXPECT_EQ(model.rowCount(), 3);
}

TEST_F(IntTableModelTest, InsertRowsOverrideZeroCountSucceedsWithoutSignal)
{
    model.resize_columns(1);
    model.push_back_row(1);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insertRows(0, 0));
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, InsertRowsOverrideRejectsInvalidArguments)
{
    model.resize_columns(1);
    model.push_back_row(1);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_FALSE(model.insertRows(-1, 1));
    EXPECT_FALSE(model.insertRows(2, 1));
    EXPECT_FALSE(model.insertRows(0, -1));
    EXPECT_FALSE(model.insertRows(0, 1, model.index(0, 0)));

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, RemoveRowsOverrideRemovesAndEmitsSignal)
{
    model.resize_columns(1);
    model.insert_rows(0, 5, 0);
    for (int r = 0; r < 5; ++r)
        model.setData(model.index(r, 0), r, ValueRole);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    EXPECT_TRUE(model.removeRows(1, 3));

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, RemoveRowsOverrideZeroCountSucceedsWithoutSignal)
{
    model.resize_columns(1);
    model.push_back_row(1);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    EXPECT_TRUE(model.removeRows(0, 0));
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, RemoveRowsOverrideRejectsInvalidArguments)
{
    model.resize_columns(1);
    model.push_back_row(1);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);

    EXPECT_FALSE(model.removeRows(-1, 1));
    EXPECT_FALSE(model.removeRows(0, 2));
    EXPECT_FALSE(model.removeRows(1, 1));
    EXPECT_FALSE(model.removeRows(0, -1));
    EXPECT_FALSE(model.removeRows(0, 1, model.index(0, 0)));

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(IntTableModelTest, InsertColumnsOverrideShiftsExistingValues)
{
    model.resize_rows(1);
    model.resize_columns(2);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));
    EXPECT_TRUE(model.setData(model.index(0, 1), 3, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_TRUE(model.insertColumns(1, 1));

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 3);
}

TEST_F(IntTableModelTest, InsertColumnsOverrideAppendsAtEnd)
{
    model.resize_rows(1);
    model.resize_columns(1);

    EXPECT_TRUE(model.insertColumns(model.columnCount(), 2));
    EXPECT_EQ(model.columnCount(), 3);
}

TEST_F(IntTableModelTest, InsertColumnsOverrideRejectsInvalidArguments)
{
    model.resize_rows(1);
    model.resize_columns(1);

    QSignalSpy spy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_FALSE(model.insertColumns(-1, 1));
    EXPECT_FALSE(model.insertColumns(2, 1));
    EXPECT_FALSE(model.insertColumns(0, -1));
    EXPECT_FALSE(model.insertColumns(0, 1, model.index(0, 0)));
    EXPECT_TRUE(model.insertColumns(0, 0));

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.columnCount(), 1);
}

TEST_F(IntTableModelTest, RemoveColumnsOverrideRemovesAndEmitsSignal)
{
    model.resize_rows(1);
    model.resize_columns(5);
    for (int c = 0; c < 5; ++c)
        EXPECT_TRUE(model.setData(model.index(0, c), c, ValueRole));

    QSignalSpy spy(&model, &QAbstractItemModel::columnsRemoved);

    EXPECT_TRUE(model.removeColumns(1, 3));

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, RemoveColumnsOverrideRejectsInvalidArguments)
{
    model.resize_rows(1);
    model.resize_columns(1);

    QSignalSpy spy(&model, &QAbstractItemModel::columnsRemoved);

    EXPECT_FALSE(model.removeColumns(-1, 1));
    EXPECT_FALSE(model.removeColumns(0, 2));
    EXPECT_FALSE(model.removeColumns(1, 1));
    EXPECT_FALSE(model.removeColumns(0, -1));
    EXPECT_FALSE(model.removeColumns(0, 1, model.index(0, 0)));
    EXPECT_TRUE(model.removeColumns(0, 0));

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model.columnCount(), 1);
}

TEST_F(IntTableModelTest, GetRefTracksCellValue)
{
    model.resize_rows(1);
    model.resize_columns(1);
    EXPECT_TRUE(model.setData(model.index(0, 0), 1, ValueRole));

    auto ref = model.getRef<>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);
    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);

    EXPECT_TRUE(model.setData(model.index(0, 0), 2, ValueRole));

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), 2);
}

// ===========================================================================
// Auto-padding behavior: insert_range_row / insert_range_column / reset with
// non-rectangular inputs, plus non-default-constructible T short-circuits.
// ===========================================================================

namespace {

using cute_tests::NonDefault;

class NonDefaultTableModel : public BasicTableModel<NonDefault>
{
public:
    using BasicTableModel<NonDefault>::BasicTableModel;
    using BasicTableModel<NonDefault>::data;

    QVariant data(const NonDefault &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value.value;
        return {};
    }
};

std::vector<std::vector<NonDefault>> nonDefaultRows(
    std::initializer_list<std::initializer_list<int>> rows)
{
    std::vector<std::vector<NonDefault>> out;
    out.reserve(rows.size());
    for (const auto &row : rows) {
        std::vector<NonDefault> r;
        r.reserve(row.size());
        for (int v : row)
            r.emplace_back(v);
        out.push_back(std::move(r));
    }
    return out;
}

}

// ----- A. Overrides Qt sur T non default-constructible -----

TEST(NonDefaultTableModel, InsertRowsReturnsFalseWithoutDefaultCtor)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_FALSE(model.insertRows(0, 1));
    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(NonDefaultTableModel, InsertRowsZeroCountStillSucceeds)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insertRows(0, 0));
    EXPECT_EQ(insertedSpy.count(), 0);
}

TEST(NonDefaultTableModel, InsertColumnsReturnsFalseWithoutDefaultCtor)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.reset(nonDefaultRows({{1, 2}}));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_FALSE(model.insertColumns(0, 1));
    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.columnCount(), 2);
}

TEST(NonDefaultTableModel, InsertColumnsZeroCountStillSucceeds)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};

    EXPECT_TRUE(model.insertColumns(0, 0));
}

TEST_F(IntTableModelTest, InsertRowsSucceedsForDefaultConstructibleT)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insertRows(0, 2));
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 1);
}

TEST_F(IntTableModelTest, InsertColumnsSucceedsForDefaultConstructibleT)
{
    model.reset(std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_TRUE(model.insertColumns(0, 2));
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.columnCount(), 4);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 1);
}

// ----- B. insert_range_row — agrandissement (incoming WIDER que table) -----

TEST_F(IntTableModelTest, InsertRangeRowWiderThanTableExtendsColumns)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy colsAboutSpy(&model, &QAbstractItemModel::columnsAboutToBeInserted);
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsAboutSpy(&model, &QAbstractItemModel::rowsAboutToBeInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insert_range_row(1,
        std::vector<std::vector<int>>{{7, 8, 9, 10, 11}}));

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.columnCount(), 5);
    ASSERT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(colsAboutSpy.count(), 1);
    EXPECT_EQ(colsSpy.at(0).at(1).toInt(), 3);
    EXPECT_EQ(colsSpy.at(0).at(2).toInt(), 4);
    ASSERT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(rowsAboutSpy.count(), 1);
    EXPECT_EQ(rowsSpy.at(0).at(1).toInt(), 1);
    EXPECT_EQ(rowsSpy.at(0).at(2).toInt(), 1);

    // Existing rows padded with 0 in new columns.
    EXPECT_EQ(model.index(0, 3).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(0, 4).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(2, 3).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(2, 4).data(ValueRole).toInt(), 0);
    // New row carries provided values.
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(1, 4).data(ValueRole).toInt(), 11);
}

TEST_F(IntTableModelTest, InsertRangeRowMultipleWiderRowsExtendsColumnsOnce)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insert_range_row(0,
        std::vector<std::vector<int>>{{7, 8, 9, 10, 11}, {12, 13, 14, 15, 16}}));

    EXPECT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.columnCount(), 5);
}

TEST_F(IntTableModelTest, InsertRangeRowOnEmptyTableEstablishesColumns)
{
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insert_range_row(0,
        std::vector<std::vector<int>>{{1, 2, 3, 4}}));

    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.columnCount(), 4);
    ASSERT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(colsSpy.at(0).at(1).toInt(), 0);
    EXPECT_EQ(colsSpy.at(0).at(2).toInt(), 3);
    ASSERT_EQ(rowsSpy.count(), 1);
}

TEST(NonDefaultTableModel, InsertRangeRowWiderThanTableReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2, 3}, {4, 5, 6}})));

    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_FALSE(model.insert_range_row(1, nonDefaultRows({{7, 8, 9, 10, 11}})));

    EXPECT_EQ(colsSpy.count(), 0);
    EXPECT_EQ(rowsSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
}

// ----- C. insert_range_row — réduction (incoming NARROWER que table) -----

TEST_F(IntTableModelTest, InsertRangeRowNarrowerPadsIncoming)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insert_range_row(1, std::vector<std::vector<int>>{{7, 8}}));

    EXPECT_EQ(colsSpy.count(), 0);
    ASSERT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 8);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 0);
}

TEST_F(IntTableModelTest, InsertRangeRowMixedNarrowerWidthsPadsEach)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    EXPECT_TRUE(model.insert_range_row(2,
        std::vector<std::vector<int>>{{7, 8}, {9}}));

    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(2, 1).data(ValueRole).toInt(), 8);
    EXPECT_EQ(model.index(2, 2).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(3, 0).data(ValueRole).toInt(), 9);
    EXPECT_EQ(model.index(3, 1).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(3, 2).data(ValueRole).toInt(), 0);
}

TEST(NonDefaultTableModel, InsertRangeRowNarrowerReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2, 3}, {4, 5, 6}})));

    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_FALSE(model.insert_range_row(1, nonDefaultRows({{7, 8}})));

    EXPECT_EQ(rowsSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 2);
}

// ----- D. insert_range_row — incoming NON RECTANGULAIRE -----

TEST_F(IntTableModelTest, InsertRangeRowNonRectangularExtendsAndPads)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_TRUE(model.insert_range_row(2,
        std::vector<std::vector<int>>{{10, 20, 30, 40}, {50, 60}}));

    ASSERT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(colsSpy.at(0).at(1).toInt(), 3);
    EXPECT_EQ(colsSpy.at(0).at(2).toInt(), 3);
    ASSERT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.columnCount(), 4);

    // Existing rows padded.
    EXPECT_EQ(model.index(0, 3).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 3).data(ValueRole).toInt(), 0);
    // First new row keeps its values.
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 10);
    EXPECT_EQ(model.index(2, 3).data(ValueRole).toInt(), 40);
    // Second new row padded with 0s.
    EXPECT_EQ(model.index(3, 0).data(ValueRole).toInt(), 50);
    EXPECT_EQ(model.index(3, 1).data(ValueRole).toInt(), 60);
    EXPECT_EQ(model.index(3, 2).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(3, 3).data(ValueRole).toInt(), 0);
}

TEST(NonDefaultTableModel, InsertRangeRowNonRectangularReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2, 3}, {4, 5, 6}})));

    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);

    EXPECT_FALSE(model.insert_range_row(2,
        nonDefaultRows({{10, 20, 30, 40}, {50, 60}})));

    EXPECT_EQ(colsSpy.count(), 0);
    EXPECT_EQ(rowsSpy.count(), 0);
}

// ----- E. Cas dégénérés -----

TEST_F(IntTableModelTest, InsertRangeRowEmptyVectorReturnsFalse)
{
    model.reset(std::vector<std::vector<int>>{{1, 2}});
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);
    EXPECT_FALSE(model.insert_range_row(0, std::vector<std::vector<int>>{}));
    EXPECT_EQ(rowsSpy.count(), 0);
}

TEST_F(IntTableModelTest, InsertRangeRowOutOfBoundsRow)
{
    model.reset(std::vector<std::vector<int>>{{1, 2}});
    EXPECT_FALSE(model.insert_range_row(-1, std::vector<std::vector<int>>{{3, 4}}));
    EXPECT_FALSE(model.insert_range_row(99, std::vector<std::vector<int>>{{3, 4}}));
    EXPECT_EQ(model.rowCount(), 1);
}

// ----- F. append_range_row variantes -----

TEST_F(IntTableModelTest, AppendRangeRowSingleVectorReturnsBool)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}});
    EXPECT_TRUE(model.append_range_row(std::vector<int>{4, 5, 6}));
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
}

TEST_F(IntTableModelTest, AppendRangeRowWiderExtendsTable)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}});
    EXPECT_TRUE(model.append_range_row(std::vector<int>{4, 5, 6, 7, 8}));
    EXPECT_EQ(model.columnCount(), 5);
    EXPECT_EQ(model.index(0, 4).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 4).data(ValueRole).toInt(), 8);
}

TEST_F(IntTableModelTest, AppendRangeRowNarrowerPads)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}});
    EXPECT_TRUE(model.append_range_row(std::vector<int>{9}));
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 9);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 0);
}

// ----- H. insert_range_column — agrandissement (incoming TALLER que table) -----

TEST_F(IntTableModelTest, InsertRangeColumnTallerExtendsRows)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});

    QSignalSpy rowsAboutSpy(&model, &QAbstractItemModel::rowsAboutToBeInserted);
    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy colsAboutSpy(&model, &QAbstractItemModel::columnsAboutToBeInserted);
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_TRUE(model.insert_range_column(1,
        std::vector<std::vector<int>>{{7, 8, 9, 10}}));

    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.columnCount(), 4);
    ASSERT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(rowsAboutSpy.count(), 1);
    EXPECT_EQ(rowsSpy.at(0).at(1).toInt(), 2);
    EXPECT_EQ(rowsSpy.at(0).at(2).toInt(), 3);
    ASSERT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(colsAboutSpy.count(), 1);
    EXPECT_EQ(colsSpy.at(0).at(1).toInt(), 1);
    EXPECT_EQ(colsSpy.at(0).at(2).toInt(), 1);

    // New rows fully default-constructed except in the new column.
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(3, 3).data(ValueRole).toInt(), 0);
    // New column carries values across all rows (4 rows total).
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 7);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 8);
    EXPECT_EQ(model.index(2, 1).data(ValueRole).toInt(), 9);
    EXPECT_EQ(model.index(3, 1).data(ValueRole).toInt(), 10);
}

TEST(NonDefaultTableModel, InsertRangeColumnTallerReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2, 3}, {4, 5, 6}})));

    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_FALSE(model.insert_range_column(1, nonDefaultRows({{7, 8, 9, 10}})));

    EXPECT_EQ(rowsSpy.count(), 0);
    EXPECT_EQ(colsSpy.count(), 0);
}

// ----- I. insert_range_column — réduction (incoming SHORTER que table) -----

TEST_F(IntTableModelTest, InsertRangeColumnShorterPadsIncoming)
{
    model.reset(std::vector<std::vector<int>>{
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});

    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_TRUE(model.insert_range_column(1,
        std::vector<std::vector<int>>{{99, 88}}));

    EXPECT_EQ(rowsSpy.count(), 0);
    ASSERT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.columnCount(), 4);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 99);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 88);
    EXPECT_EQ(model.index(2, 1).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(3, 1).data(ValueRole).toInt(), 0);
}

TEST(NonDefaultTableModel, InsertRangeColumnShorterReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows(
        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}})));

    EXPECT_FALSE(model.insert_range_column(1, nonDefaultRows({{99, 88}})));
    EXPECT_EQ(model.columnCount(), 3);
}

// ----- J. insert_range_column — non rectangulaire -----

TEST_F(IntTableModelTest, InsertRangeColumnNonRectangularExtendsAndPads)
{
    model.reset(std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    QSignalSpy rowsSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy colsSpy(&model, &QAbstractItemModel::columnsInserted);

    EXPECT_TRUE(model.insert_range_column(0,
        std::vector<std::vector<int>>{{10, 20, 30}, {40}}));

    EXPECT_EQ(rowsSpy.count(), 1);
    EXPECT_EQ(colsSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.columnCount(), 4);

    // Existing row 0 keeps {1,2} in shifted columns; row 0 column 2,3 is shifted-original.
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 3).data(ValueRole).toInt(), 2);
    // First new column carries {10,20,30}.
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 10);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 20);
    EXPECT_EQ(model.index(2, 0).data(ValueRole).toInt(), 30);
    // Second new column carries {40, padding, padding}.
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 40);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(2, 1).data(ValueRole).toInt(), 0);
    // Padded row (row 2) of the original columns is 0.
    EXPECT_EQ(model.index(2, 2).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(2, 3).data(ValueRole).toInt(), 0);
}

TEST(NonDefaultTableModel, InsertRangeColumnNonRectangularReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2}, {3, 4}})));

    EXPECT_FALSE(model.insert_range_column(0,
        nonDefaultRows({{10, 20, 30}, {40}})));
}

// ----- K. reset exhaustif -----

TEST_F(IntTableModelTest, ResetEmptyProducesZeroByZero)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}});

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    EXPECT_TRUE(model.reset({}));

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 0);
}

TEST_F(IntTableModelTest, ResetRectangular)
{
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    EXPECT_TRUE(model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}}));

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
}

TEST_F(IntTableModelTest, ResetNonRectangularPadsToMaxWidth)
{
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    EXPECT_TRUE(model.reset(std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}}));

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 1);
    EXPECT_EQ(model.index(0, 1).data(ValueRole).toInt(), 2);
    EXPECT_EQ(model.index(0, 2).data(ValueRole).toInt(), 0);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 3);
    EXPECT_EQ(model.index(1, 1).data(ValueRole).toInt(), 4);
    EXPECT_EQ(model.index(1, 2).data(ValueRole).toInt(), 5);
}

TEST_F(IntTableModelTest, ResetMixedWidePadsToMaxWidth)
{
    EXPECT_TRUE(model.reset(std::vector<std::vector<int>>{{1, 2, 3, 4, 5}, {6}}));

    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 5);
    EXPECT_EQ(model.index(1, 0).data(ValueRole).toInt(), 6);
    EXPECT_EQ(model.index(1, 4).data(ValueRole).toInt(), 0);
}

TEST(NonDefaultTableModel, ResetNonRectangularReturnsFalse)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2}, {3, 4}})));

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    EXPECT_FALSE(model.reset(nonDefaultRows({{5}, {6, 7, 8}})));

    EXPECT_EQ(resetSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 2);
}

TEST(NonDefaultTableModel, ResetRectangularSucceeds)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    EXPECT_TRUE(model.reset(nonDefaultRows({{1, 2, 3}, {4, 5, 6}})));

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toInt(), 1);
}

TEST(NonDefaultTableModel, ClearOnEmptyStaysEmpty)
{
    NonDefaultTableModel model;
    QAbstractItemModelTester guard{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 0);
}

// ----- L. Suppression / réduction des dimensions existantes -----

TEST_F(IntTableModelTest, RemoveRowsReducesRowCount)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});
    EXPECT_TRUE(model.removeRows(0, 1));
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 4);
}

TEST_F(IntTableModelTest, RemoveColumnsReducesColumnCount)
{
    model.reset(std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}});
    EXPECT_TRUE(model.removeColumns(0, 1));
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 2);
}

TEST_F(IntTableModelTest, ResizeRowsShrinksAndGrows)
{
    model.resize_columns(2);
    model.resize_rows(3, 7);
    EXPECT_EQ(model.rowCount(), 3);
    model.resize_rows(1);
    EXPECT_EQ(model.rowCount(), 1);
    model.resize_rows(4, 9);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.index(3, 0).data(ValueRole).toInt(), 9);
}
