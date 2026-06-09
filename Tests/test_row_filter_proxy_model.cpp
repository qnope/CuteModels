#include "CuteModel/BasicListModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/RowFilterProxyModel.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QModelIndex>
#include <QSignalSpy>
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

// --- Pass-through (aucun prédicat) ------------------------------------------

TEST_F(RowFilterProxyModelTest, MirrorsSourceWithoutPredicate)
{
    EXPECT_FALSE(proxy.hasFilter());
    EXPECT_EQ(proxy.typedSourceModel(), &source);
    EXPECT_EQ(proxy.rowCount(), source.rowCount());
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

// --- Filtrage ---------------------------------------------------------------

TEST_F(RowFilterProxyModelTest, PredicateReducesRowsAndKeepsValues)
{
    proxy.setFilterPredicate(isEven);

    EXPECT_TRUE(proxy.hasFilter());
    ASSERT_EQ(proxy.rowCount(), 3);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    for (int row = 0; row < proxy.rowCount(); ++row) {
        const QModelIndex index = proxy.index(row, 0);
        EXPECT_EQ(proxy.data(index, ValueRole).toInt(), proxy.getStorageValue(index));
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

// --- Correspondance d'index -------------------------------------------------

TEST_F(RowFilterProxyModelTest, IndexMappingRoundTrips)
{
    proxy.setFilterPredicate(isEven);

    const QModelIndex proxyIndex = proxy.index(1, 0); // valeur 4
    const QModelIndex sourceIndex = proxy.mapToSource(proxyIndex);

    EXPECT_EQ(sourceIndex.row(), 3); // 4 est à la ligne 3 de la source
    EXPECT_EQ(proxy.mapFromSource(sourceIndex), proxyIndex);
    EXPECT_EQ(proxy.getStorageValue(proxyIndex), 4);
}

// --- Refs typées ------------------------------------------------------------

TEST_F(RowFilterProxyModelTest, TypedRefReadsAndWritesThroughFilter)
{
    proxy.setFilterPredicate(isEven);

    auto ref = proxy.getRef(proxy.index(1, 0)); // valeur 4, ligne source 3
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), 4);

    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);
    ref->setValue(40); // reste pair : la ligne survit au filtre

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), 40);
    EXPECT_EQ(*source.at(3), 40);
}

// --- Re-filtrage dynamique --------------------------------------------------

TEST_F(RowFilterProxyModelTest, AcceptedSourceInsertEmitsProxyRowsInserted)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    QSignalSpy insertedSpy(&proxy, &QAbstractItemModel::rowsInserted);

    source.push_back(8); // pair : visible
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(proxy.rowCount(), 4);

    source.push_back(7); // impair : filtré
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(proxy.rowCount(), 4);
}

TEST_F(RowFilterProxyModelTest, RemovingVisibleSourceRowEmitsProxyRowsRemoved)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    QSignalSpy removedSpy(&proxy, &QAbstractItemModel::rowsRemoved);

    source.erase(1); // supprime la valeur 2 (visible)
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));

    source.erase(0); // supprime la valeur 1 (filtrée)
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));
}

TEST_F(RowFilterProxyModelTest, EditMakingRowFailPredicateRemovesItFromProxy)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxyValues(proxy), (std::vector<int>{2, 4, 6}));

    QSignalSpy removedSpy(&proxy, &QAbstractItemModel::rowsRemoved);

    source.setData(source.index(1, 0), 3, ValueRole); // 2 -> 3 : devient impair

    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{4, 6}));
}

// --- Changement de prédicat -------------------------------------------------

TEST_F(RowFilterProxyModelTest, ChangingPredicateUpdatesRowCount)
{
    proxy.setFilterPredicate(isEven);
    ASSERT_EQ(proxy.rowCount(), 3);

    proxy.setFilterPredicate([](const int &value, const QModelIndex &) { return value > 4; });

    EXPECT_EQ(proxyValues(proxy), (std::vector<int>{5, 6}));
}

// --- Source invalide --------------------------------------------------------

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
