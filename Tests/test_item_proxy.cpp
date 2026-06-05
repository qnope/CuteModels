#include "CuteModel/BasicListModel.h"
#include "CuteModel/ItemProxy.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QString>

#include <stdexcept>

using cute::BasicListModel;
using cute::ItemProxy;

namespace {

class IntListModel : public BasicListModel<int>
{
public:
    using BasicListModel<int>::BasicListModel;
    using BasicListModel<int>::data;

    QVariant data(const int &value, const QModelIndex &index, int role) const override
    {
        if (index.column() == 0 && role == Qt::DisplayRole)
            return value;
        return {};
    }
};

}

TEST(ItemProxyCtor, ValidIndicesSucceeds)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(42);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    int value = 42;
    {
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)));
        EXPECT_EQ(*proxy, 42);
    }

    EXPECT_EQ(changedSpy.count(), 1);
}

TEST(ItemProxyCtor, InvalidIndexThrows)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(42);

    int value = 42;
    EXPECT_THROW({
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)));
    }, std::invalid_argument);
}

TEST(ItemProxyCtor, InvalidLeftThrows)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(42);

    int value = 42;
    EXPECT_THROW({
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(),
                             QPersistentModelIndex(model.index(0, 0)));
    }, std::invalid_argument);
}

TEST(ItemProxyCtor, InvalidRightThrows)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(42);

    int value = 42;
    EXPECT_THROW({
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex());
    }, std::invalid_argument);
}

TEST(ItemProxyCtor, AllInvalidThrows)
{
    int value = 42;
    EXPECT_THROW({
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(),
                             QPersistentModelIndex(),
                             QPersistentModelIndex());
    }, std::invalid_argument);
}

TEST(ItemProxyCtor, ConstProxyDoesNotEmitOnDestruction)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(42);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    const int value = 42;
    {
        ItemProxy<const int> proxy(value,
                                   QPersistentModelIndex(model.index(0, 0)),
                                   QPersistentModelIndex(model.index(0, 0)),
                                   QPersistentModelIndex(model.index(0, 0)));
        EXPECT_EQ(*proxy, 42);
    }

    EXPECT_EQ(changedSpy.count(), 0);
}

TEST(ItemProxyCtor, MutableProxyAssignmentChangesValue)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    int value = 1;
    {
        ItemProxy<int> proxy(value,
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)));
        proxy = 99;
        EXPECT_EQ(*proxy, 99);
    }

    EXPECT_EQ(value, 99);
    EXPECT_EQ(changedSpy.count(), 1);
}
