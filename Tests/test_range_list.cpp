#include "CuteModel/BasicListModel.h"
#include "CuteModel/RangeList.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QString>

#include <stdexcept>
#include <vector>

using cute::BasicListModel;
using cute::RangeList;

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

TEST(RangeListCtor, EmptyRangeWithInvalidIndicesSucceeds)
{
    std::vector<int> data;
    EXPECT_NO_THROW({
        RangeList<int> range(data.begin(), data.end(),
                             QPersistentModelIndex(), QPersistentModelIndex());
        (void)range.empty();
    });
}

TEST(RangeListCtor, EmptyRangeWithValidIndicesSucceeds)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);

    std::vector<int> data{1};
    EXPECT_NO_THROW({
        RangeList<int> range(data.begin(), data.begin(),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(0, 0)));
        EXPECT_TRUE(range.empty());
    });
}

TEST(RangeListCtor, NonEmptyRangeWithValidIndicesSucceeds)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);
    model.push_back(2);
    model.push_back(3);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        // Use a snapshot vector to keep the iterators valid for the lifetime
        // of the RangeList. The indices belong to the model so the destructor
        // can legally emit dataChanged on it.
        std::vector<int> snapshot{1, 2, 3};
        RangeList<int> range(snapshot.begin(), snapshot.end(),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex(model.index(2, 0)));
        EXPECT_FALSE(range.empty());
        EXPECT_EQ(range.size(), 3);
    }

    EXPECT_EQ(changedSpy.count(), 1);
}

TEST(RangeListCtor, NonEmptyRangeWithInvalidTopLeftThrows)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);
    model.push_back(2);

    std::vector<int> data{1, 2};
    EXPECT_THROW({
        RangeList<int> range(data.begin(), data.end(),
                             QPersistentModelIndex(),
                             QPersistentModelIndex(model.index(1, 0)));
    }, std::invalid_argument);
}

TEST(RangeListCtor, NonEmptyRangeWithInvalidBottomRightThrows)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);
    model.push_back(2);

    std::vector<int> data{1, 2};
    EXPECT_THROW({
        RangeList<int> range(data.begin(), data.end(),
                             QPersistentModelIndex(model.index(0, 0)),
                             QPersistentModelIndex());
    }, std::invalid_argument);
}

TEST(RangeListCtor, NonEmptyRangeWithBothInvalidThrows)
{
    std::vector<int> data{1, 2};
    EXPECT_THROW({
        RangeList<int> range(data.begin(), data.end(),
                             QPersistentModelIndex(), QPersistentModelIndex());
    }, std::invalid_argument);
}

TEST(RangeListCtor, ConstRangeDoesNotEmitOnDestruction)
{
    IntListModel model;
    QAbstractItemModelTester tester{&model,
        QAbstractItemModelTester::FailureReportingMode::Fatal};
    model.push_back(1);
    model.push_back(2);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        const std::vector<int> snapshot{1, 2};
        RangeList<const int> range(snapshot.cbegin(), snapshot.cend(),
                                   QPersistentModelIndex(model.index(0, 0)),
                                   QPersistentModelIndex(model.index(1, 0)));
        EXPECT_EQ(range.size(), 2);
    }

    EXPECT_EQ(changedSpy.count(), 0);
}
