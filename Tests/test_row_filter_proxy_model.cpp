#include "CuteModel/BasicListModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/RowFilterProxyModel.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QVariant>

#include <stdexcept>
#include <vector>

using cute::BasicListModel;
using cute::RowFilterProxyModel;
using cute::ValueRole;

namespace {

using Reporting = QAbstractItemModelTester::FailureReportingMode;

class IntListModel : public BasicListModel<int>
{
public:
    using BasicListModel<int>::BasicListModel;
    using BasicListModel<int>::data;

    QVariant data(const int &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value;
        return {};
    }
};

class TwoColumnIntModel : public BasicListModel<int>
{
public:
    TwoColumnIntModel()
        : BasicListModel<int>(QStringList{QStringLiteral("value"), QStringLiteral("times10")})
    {}

    using BasicListModel<int>::data;

    QVariant data(const int &value, const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        return index.column() == 0 ? value : value * 10;
    }
};

bool isEven(const int &value, const QModelIndex &)
{
    return value % 2 == 0;
}

std::vector<int> proxyValues(const RowFilterProxyModel<int> &proxy)
{
    std::vector<int> values;
    for (int row = 0; row < proxy.rowCount(); ++row)
        values.push_back(proxy.getStorageValue(proxy.index(row, 0)));
    return values;
}

class RowFilterProxyModelTest : public ::testing::Test
{
protected:
    IntListModel source;
    RowFilterProxyModel<int> proxy{&source};
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};

    void SetUp() override { source.reset({1, 2, 3, 4, 5, 6}); }
};

}

TEST_F(RowFilterProxyModelTest, MirrorsSourceWithoutPredicate)
{
    EXPECT_FALSE(proxy.hasFilter());
    EXPECT_EQ(proxy.typedSourceModel(), &source);
    EXPECT_EQ(proxy.rowCount(), source.rowCount());
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST_F(RowFilterProxyModelTest, PredicateReducesRowsAndKeepsValues)
{
    proxy.setFilterPredicate(isEven);

    EXPECT_TRUE(proxy.hasFilter());
    ASSERT_EQ(proxy.rowCount(), 3);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    for (int row = 0; row < proxy.rowCount(); ++row) {
        const QModelIndex index = proxy.index(row, 0);
        EXPECT_EQ(proxy.data(index, ValueRole).toInt(), proxy.getStorageValue(index));
        EXPECT_EQ(proxy.data(index, Qt::DisplayRole).toInt(), proxy.getStorageValue(index));
    }
}

TEST_F(RowFilterProxyModelTest, ClearFilterRestoresAllRows)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    proxy.clearFilter();

    EXPECT_FALSE(proxy.hasFilter());
    EXPECT_EQ(proxy.rowCount(), 6);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST_F(RowFilterProxyModelTest, AcceptAllPredicateKeepsEveryRow)
{
    proxy.setFilterPredicate([](const int &, const QModelIndex &) { return true; });

    EXPECT_EQ(proxy.rowCount(), 6);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST_F(RowFilterProxyModelTest, RejectAllPredicateEmptiesProxy)
{
    proxy.setFilterPredicate([](const int &, const QModelIndex &) { return false; });

    EXPECT_EQ(proxy.rowCount(), 0);
    EXPECT_TRUE(proxyValues(proxy).empty());
}

TEST_F(RowFilterProxyModelTest, IndexMappingRoundTrips)
{
    proxy.setFilterPredicate(isEven);

    const QModelIndex proxyIndex = proxy.index(1, 0);
    const QModelIndex sourceIndex = proxy.mapToSource(proxyIndex);

    EXPECT_EQ(sourceIndex.row(), 3);
    EXPECT_EQ(proxy.mapFromSource(sourceIndex), proxyIndex);
    EXPECT_EQ(proxy.getStorageValue(proxyIndex), 4);
}

TEST_F(RowFilterProxyModelTest, FilteredOutSourceRowMapsToInvalidProxyIndex)
{
    proxy.setFilterPredicate(isEven);

    const QModelIndex oddSource = source.index(0, 0);
    EXPECT_EQ(*source.at(0), 1);
    EXPECT_FALSE(proxy.mapFromSource(oddSource).isValid());
}

TEST_F(RowFilterProxyModelTest, TypedRefReadsAndWritesThroughFilter)
{
    proxy.setFilterPredicate(isEven);

    auto ref = proxy.getRef(proxy.index(1, 0));
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), 4);

    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);
    ref->setValue(40);

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), 40);
    EXPECT_EQ(*source.at(3), 40);
}

TEST_F(RowFilterProxyModelTest, GetRefOnInvalidIndexReturnsNull)
{
    EXPECT_EQ(proxy.getRef(QModelIndex()), nullptr);
}

TEST_F(RowFilterProxyModelTest, SetStorageValueThroughProxyUpdatesSource)
{
    proxy.setFilterPredicate(isEven);

    const QModelIndex proxyIndex = proxy.index(0, 0);
    ASSERT_EQ(proxy.getStorageValue(proxyIndex), 2);

    proxy.setStorageValue(proxyIndex, 20);

    EXPECT_EQ(proxy.getStorageValue(proxy.index(0, 0)), 20);
    EXPECT_EQ(*source.at(1), 20);
}

TEST_F(RowFilterProxyModelTest, SetDataThroughProxyForwardsToSource)
{
    const QModelIndex proxyIndex = proxy.index(2, 0);
    ASSERT_EQ(proxy.getStorageValue(proxyIndex), 3);

    QSignalSpy changedSpy(&proxy, &QAbstractItemModel::dataChanged);
    EXPECT_TRUE(proxy.setData(proxyIndex, 33, ValueRole));

    EXPECT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(*source.at(2), 33);
}

TEST_F(RowFilterProxyModelTest, FlagsAreForwardedFromSource)
{
    EXPECT_EQ(proxy.flags(proxy.index(0, 0)),
              source.flags(source.index(0, 0)));
    EXPECT_EQ(proxy.flags(QModelIndex()), source.flags(QModelIndex()));
}

TEST_F(RowFilterProxyModelTest, AcceptedSourceInsertEmitsProxyRowsInserted)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    QSignalSpy insertedSpy(&proxy, &QAbstractItemModel::rowsInserted);

    source.push_back(8);
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(proxy.rowCount(), 4);

    source.push_back(7);
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(proxy.rowCount(), 4);
}

TEST_F(RowFilterProxyModelTest, RemovingVisibleSourceRowEmitsProxyRowsRemoved)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    QSignalSpy removedSpy(&proxy, &QAbstractItemModel::rowsRemoved);

    source.erase(1);
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));

    source.erase(0);
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));
}

TEST_F(RowFilterProxyModelTest, EditMakingRowFailPredicateRemovesItFromProxy)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    QSignalSpy removedSpy(&proxy, &QAbstractItemModel::rowsRemoved);

    source.setData(source.index(1, 0), 3, ValueRole);

    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));
}

TEST_F(RowFilterProxyModelTest, EditMakingRowPassPredicateAddsItToProxy)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    QSignalSpy insertedSpy(&proxy, &QAbstractItemModel::rowsInserted);

    source.setData(source.index(0, 0), 8, ValueRole);

    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{8, 2, 4, 6}));
}

TEST_F(RowFilterProxyModelTest, ChangingPredicateUpdatesRowCount)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    proxy.setFilterPredicate([](const int &value, const QModelIndex &) { return value > 4; });

    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{5, 6}));
}

TEST_F(RowFilterProxyModelTest, SourceResetIsReflectedUnderFilter)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    source.reset({10, 11, 12, 13});

    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{10, 12}));
}

TEST_F(RowFilterProxyModelTest, PredicateReceivesSourceIndex)
{
    proxy.setFilterPredicate([this](const int &value, const QModelIndex &index) {
        return index.model() == &source && value % 2 == 0;
    });

    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));
}

TEST(RowFilterProxyModelColumnsTest, FilteringPreservesColumnsAndData)
{
    TwoColumnIntModel source;
    source.reset({1, 2, 3, 4});
    RowFilterProxyModel<int> proxy{&source};
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};

    proxy.setFilterPredicate(isEven);

    ASSERT_EQ(proxy.rowCount(), 2);
    ASSERT_EQ(proxy.columnCount(), 2);

    EXPECT_EQ(proxy.data(proxy.index(0, 0), Qt::DisplayRole).toInt(), 2);
    EXPECT_EQ(proxy.data(proxy.index(0, 1), Qt::DisplayRole).toInt(), 20);
    EXPECT_EQ(proxy.data(proxy.index(1, 0), Qt::DisplayRole).toInt(), 4);
    EXPECT_EQ(proxy.data(proxy.index(1, 1), Qt::DisplayRole).toInt(), 40);
}

TEST(RowFilterProxyModelColumnsTest, HeaderDataIsForwarded)
{
    TwoColumnIntModel source;
    source.reset({2, 4});
    RowFilterProxyModel<int> proxy{&source};
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};

    EXPECT_EQ(proxy.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("value"));
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("times10"));
}

TEST(RowFilterProxyModelSourceTest, EmptySourceGivesEmptyProxy)
{
    IntListModel source;
    RowFilterProxyModel<int> proxy{&source};
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};

    proxy.setFilterPredicate(isEven);

    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(RowFilterProxyModelSourceTest, ReplacingSourceModelRetargetsProxy)
{
    IntListModel first;
    first.reset({1, 2, 3});
    IntListModel second;
    second.reset({10, 20, 30, 40});

    RowFilterProxyModel<int> proxy{&first};
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};
    ASSERT_EQ(proxy.rowCount(), 3);

    proxy.setSourceModel(&second);

    EXPECT_EQ(proxy.typedSourceModel(), &second);
    EXPECT_EQ(proxy.rowCount(), 4);
    EXPECT_EQ(proxy.getStorageValue(proxy.index(2, 0)), 30);
}

TEST(RowFilterProxyModelSourceTest, ConstructedWithoutSourceThenAssigned)
{
    RowFilterProxyModel<int> proxy;
    QAbstractItemModelTester tester{&proxy, Reporting::Fatal};
    EXPECT_EQ(proxy.typedSourceModel(), nullptr);
    EXPECT_EQ(proxy.rowCount(), 0);

    IntListModel source;
    source.reset({5, 6, 7, 8});
    proxy.setSourceModel(&source);
    proxy.setFilterPredicate(isEven);

    EXPECT_EQ(proxy.typedSourceModel(), &source);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{6, 8}));
}

TEST(RowFilterProxyModelSourceTest, NonSourceModelThrows)
{
    RowFilterProxyModel<int> proxy;
    QStringListModel foreign;

    EXPECT_THROW(proxy.setSourceModel(&foreign), std::invalid_argument);
}

TEST(RowFilterProxyModelSourceTest, NullSourceIsAccepted)
{
    IntListModel source;
    source.reset({1, 2, 3});
    RowFilterProxyModel<int> proxy{&source};
    ASSERT_EQ(proxy.typedSourceModel(), &source);

    proxy.setSourceModel(static_cast<QAbstractItemModel *>(nullptr));

    EXPECT_EQ(proxy.typedSourceModel(), nullptr);
    EXPECT_EQ(proxy.rowCount(), 0);
}
