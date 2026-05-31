#include "CuteModel/BasicListModel.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>

using cute::BasicListModel;
using cute::ValueRole;

namespace {

struct Pod
{
    int id = 0;
    QString label;
};

bool operator==(const Pod &a, const Pod &b)
{
    return a.id == b.id && a.label == b.label;
}

class MultiColumnPodModel : public BasicListModel<Pod>
{
public:
    MultiColumnPodModel()
        : BasicListModel<Pod>(QStringList{QStringLiteral("ID"), QStringLiteral("Label")})
    {}

    QVariant data(const Pod &value, int column, int role) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
            return {};
        if (column == 0)
            return value.id;
        if (column == 1)
            return value.label;
        return {};
    }
};

class MultiColumnPodModelTest : public ::testing::Test
{
protected:
    MultiColumnPodModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

class SingleColumnStringModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;

    QVariant data(const QString &value, int column, int role) const override
    {
        if (column == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
            return value;
        return {};
    }
};

} // namespace

TEST_F(MultiColumnPodModelTest, ColumnCountMatchesHeaders)
{
    EXPECT_EQ(model.columnCount(), 2);
    model.push_back(Pod{1, QStringLiteral("one")});
    EXPECT_EQ(model.columnCount(model.index(0, 0)), 0);
}

TEST_F(MultiColumnPodModelTest, HeaderDataDisplaysHeaders)
{
    EXPECT_EQ(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("ID"));
    EXPECT_EQ(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("Label"));
    EXPECT_FALSE(model.headerData(0, Qt::Horizontal, Qt::EditRole).isValid());
}

TEST_F(MultiColumnPodModelTest, PerColumnDataProjection)
{
    model.push_back(Pod{42, QStringLiteral("answer")});

    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toInt(), 42);
    EXPECT_EQ(model.index(0, 1).data(Qt::DisplayRole).toString(),
              QStringLiteral("answer"));
    EXPECT_FALSE(model.index(0, 2).isValid());
}

TEST_F(MultiColumnPodModelTest, ValueRoleReturnsFullPodIndependentOfColumn)
{
    model.push_back(Pod{7, QStringLiteral("seven")});

    const Pod expected{7, QStringLiteral("seven")};
    EXPECT_TRUE(model.index(0, 0).data(ValueRole).value<Pod>() == expected);
    EXPECT_TRUE(model.index(0, 1).data(ValueRole).value<Pod>() == expected);
}

TEST_F(MultiColumnPodModelTest, FlagsDefaultEditableOnlyOnColumn0)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    EXPECT_TRUE(model.flags(model.index(0, 0)) & Qt::ItemIsEditable);
    EXPECT_FALSE(model.flags(model.index(0, 1)) & Qt::ItemIsEditable);
    EXPECT_TRUE(model.flags(model.index(0, 1)) & Qt::ItemIsSelectable);
    EXPECT_TRUE(model.flags(model.index(0, 1)) & Qt::ItemIsEnabled);
}

TEST_F(MultiColumnPodModelTest, Column0EditEmitsRowRangeDataChanged)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_TRUE(model.setData(model.index(0, 0),
                              QVariant::fromValue(Pod{2, QStringLiteral("two")}),
                              Qt::EditRole));
    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>().column(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().column(), model.columnCount() - 1);
    EXPECT_EQ(args.at(0).value<QModelIndex>().row(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().row(), 0);
}

// A row is one whole T: only the row is significant, so a write addressed
// through any column rewrites the entire row and emits a row-wide dataChanged.
// (Columns > 0 are still non-editable via flags(); see
// FlagsDefaultEditableOnlyOnColumn0.)
TEST_F(MultiColumnPodModelTest, ColumnNWriteRewritesWholeRow)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_TRUE(model.setData(model.index(0, 1),
                              QVariant::fromValue(Pod{2, QStringLiteral("two")}),
                              Qt::EditRole));
    EXPECT_TRUE(*model.at(0) == (Pod{2, QStringLiteral("two")}));
    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>().column(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().column(), model.columnCount() - 1);
    EXPECT_EQ(args.at(0).value<QModelIndex>().row(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().row(), 0);
}

TEST_F(MultiColumnPodModelTest, GetMutableEmitsRowRangeDataChanged)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.getMutable(0) = Pod{2, QStringLiteral("two")};

    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>().column(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().column(), model.columnCount() - 1);
    EXPECT_EQ(args.at(0).value<QModelIndex>().row(), 0);
    EXPECT_EQ(args.at(1).value<QModelIndex>().row(), 0);
}

TEST(BasicListModelHeadersCtor, DefaultCtorPreservesSingleColumn)
{
    SingleColumnStringModel model;
    EXPECT_EQ(model.columnCount(), 1);
    EXPECT_EQ(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QString());
}

TEST(BasicListModelHeadersCtor, EmptyHeadersListIsClampedToSingleColumn)
{
    class ClampedModel : public BasicListModel<QString>
    {
    public:
        ClampedModel() : BasicListModel<QString>(QStringList{}) {}
        QVariant data(const QString &value, int column, int role) const override
        {
            if (column == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
                return value;
            return {};
        }
    } model;
    EXPECT_EQ(model.columnCount(), 1);
}
