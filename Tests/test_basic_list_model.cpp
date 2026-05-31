#include "CuteModel/BasicListModel.h"
#include "CuteModel/GetAsRefObject.h"
#include "CuteModel/Ref.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QByteArray>
#include <QList>
#include <QMimeData>
#include <QModelIndex>
#include <QModelRoleData>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <optional>

using cute::BasicListModel;
using cute::getAsRefObject;
using cute::Ref;
using cute::ValueRole;

namespace {

class StringListModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, int column, int role) const override
    {
        if (column != 0)
            return {};
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

class PodListModel : public BasicListModel<Pod>
{
public:
    using BasicListModel<Pod>::BasicListModel;
    using BasicListModel<Pod>::data;

    QVariant data(const Pod &value, int column, int role) const override
    {
        if (column != 0)
            return {};
        if (role == Qt::DisplayRole)
            return value.label;
        return {};
    }
};

// Drop/drag-enabled subclass used by the drag/drop tests. Captures the
// arguments dropValue/canDropValue received so tests can inspect them; the
// mime payload it accepts is just a single text/plain QString.
class DroppingListModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, int column, int role) const override
    {
        if (column != 0)
            return {};
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return value;
        return {};
    }

    QMimeData *mimeDataForValue(const QString &value) const override
    {
        auto *out = new QMimeData;
        out->setText(value);
        return out;
    }

    bool canDropValue(std::optional<QString> parent,
                      std::optional<int> row,
                      int column,
                      Qt::DropAction action,
                      const QMimeData *data) const override
    {
        lastCanDropParent = parent;
        lastCanDropRow = row;
        lastCanDropColumn = column;
        lastCanDropAction = action;
        lastCanDropText = data ? std::optional<QString>(data->text()) : std::nullopt;
        return data && data->hasText();
    }

    bool dropValue(std::optional<QString> parent,
                   std::optional<int> row,
                   int column,
                   Qt::DropAction action,
                   const QMimeData *data) override
    {
        lastDropParent = parent;
        lastDropRow = row;
        lastDropColumn = column;
        lastDropAction = action;
        lastDropText = data ? std::optional<QString>(data->text()) : std::nullopt;
        if (data && data->hasText()) {
            this->push_back(data->text());
            return true;
        }
        return false;
    }

    mutable std::optional<QString> lastCanDropParent;
    mutable std::optional<int> lastCanDropRow;
    mutable int lastCanDropColumn = -42;
    mutable Qt::DropAction lastCanDropAction = Qt::IgnoreAction;
    mutable std::optional<QString> lastCanDropText;

    std::optional<QString> lastDropParent;
    std::optional<int> lastDropRow;
    int lastDropColumn = -42;
    Qt::DropAction lastDropAction = Qt::IgnoreAction;
    std::optional<QString> lastDropText;
};

class BasicListModelTest : public ::testing::Test
{
protected:
    StringListModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

class PodListModelTest : public ::testing::Test
{
protected:
    PodListModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

class DroppingListModelTest : public ::testing::Test
{
protected:
    DroppingListModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

QStringList rawStorageSnapshot(const StringListModel &model)
{
    QStringList out;
    for (const QString &s : model)
        out.push_back(s);
    return out;
}

} // namespace

TEST_F(BasicListModelTest, RowCountReflectsStorage)
{
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_TRUE(model.empty());

    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));
    model.push_back(QStringLiteral("c"));

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(model.rowCount(model.index(0, 0)), 0);
}

TEST_F(BasicListModelTest, ValueRoleRoundTrip)
{
    model.push_back(QStringLiteral("hello"));

    EXPECT_EQ(model.index(0, 0).data(ValueRole).value<QString>(),
              QStringLiteral("hello"));
}

TEST_F(PodListModelTest, ValueRoleRoundTrip)
{
    model.push_back(Pod{7, QStringLiteral("seven")});

    const QVariant valueVariant = model.index(0, 0).data(ValueRole);
    ASSERT_TRUE(valueVariant.isValid());
    EXPECT_TRUE(valueVariant.value<Pod>() == (Pod{7, QStringLiteral("seven")}));
}

TEST_F(PodListModelTest, ProjectionDrivesDisplayRole)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toString(),
              QStringLiteral("one"));
    EXPECT_FALSE(model.index(0, 0).data(9999).isValid());
}

TEST_F(BasicListModelTest, MultiDataFillsHandledRoles)
{
    model.push_back(QStringLiteral("x"));

    QModelRoleData roles[] = {
        QModelRoleData(ValueRole),
        QModelRoleData(Qt::DisplayRole),
        QModelRoleData(9999),
    };
    QModelRoleDataSpan span(roles, 3);
    model.multiData(model.index(0, 0), span);

    EXPECT_EQ(roles[0].data().value<QString>(), QStringLiteral("x"));
    EXPECT_EQ(roles[1].data().toString(), QStringLiteral("x"));
    EXPECT_FALSE(roles[2].data().isValid());
}

TEST_F(BasicListModelTest, PushBackEmitsRowsInserted)
{
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.push_back(QStringLiteral("a"));

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
}

TEST_F(PodListModelTest, EmplaceBackConstructsInPlace)
{
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    Pod &ref = model.emplace_back(Pod{42, QStringLiteral("answer")});

    EXPECT_EQ(ref.id, 42);
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.at(0)->label, QStringLiteral("answer"));
}

TEST_F(BasicListModelTest, InsertShiftsRows)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("c"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.insert(1, QStringLiteral("b"));

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("b"));
    EXPECT_EQ(*model.at(2), QStringLiteral("c"));
}

TEST_F(BasicListModelTest, AppendRangeEmitsSingleRowsInserted)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.append_range({QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d")});

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.size(), 4);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("b"));
    EXPECT_EQ(*model.at(2), QStringLiteral("c"));
    EXPECT_EQ(*model.at(3), QStringLiteral("d"));
}

TEST_F(BasicListModelTest, AppendRangeEmptyIsNoOp)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.append_range({});

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
}

TEST_F(BasicListModelTest, InsertRangeShiftsRows)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("d"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range(1, {QStringLiteral("b"), QStringLiteral("c")});

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.size(), 4);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("b"));
    EXPECT_EQ(*model.at(2), QStringLiteral("c"));
    EXPECT_EQ(*model.at(3), QStringLiteral("d"));
}

TEST_F(BasicListModelTest, InsertRangeAtEndAppends)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range(model.size(), {QStringLiteral("b"), QStringLiteral("c")});

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(*model.at(2), QStringLiteral("c"));
}

TEST_F(BasicListModelTest, InsertRangeEmptyIsNoOp)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.insert_range(0, {});

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
}

TEST_F(BasicListModelTest, SetDataEmitsDataChangedAndUpdatesValue)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_TRUE(model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole));
    EXPECT_EQ(*model.at(0), QStringLiteral("b"));
    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(0, 0));
    EXPECT_EQ(args.at(1).value<QModelIndex>(), model.index(0, 0));
}

TEST_F(PodListModelTest, SetDataRejectsIncompatibleValue)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_FALSE(model.setData(model.index(0, 0), QStringLiteral("nope"), ValueRole));
    EXPECT_EQ(changedSpy.count(), 0);
}

TEST_F(BasicListModelTest, GetMutableEmitsDataChangedOnCommit)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.getMutable(0) = QStringLiteral("z");
    EXPECT_EQ(*model.at(0), QStringLiteral("z"));
    EXPECT_EQ(changedSpy.count(), 1);

    QString read = *model[0];
    EXPECT_EQ(read, QStringLiteral("z"));
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, GetMutableDrivesRefValueChanged)
{
    model.push_back(QStringLiteral("a"));

    Ref<QString> ref(QPersistentModelIndex(model.index(0, 0)));
    QSignalSpy valueChangedSpy(&ref, &cute::RefBase::valueChanged);

    model.getMutable(0) = QStringLiteral("new");

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref.getValue(), QStringLiteral("new"));
}

TEST_F(BasicListModelTest, GetMutableDefersDataChangedUntilDestruction)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto ref = model.getMutable(0);
        ref = QStringLiteral("b");
        // Storage is updated in line (the proxy holds a live reference),
        // but no dataChanged has fired yet — the RAII emit only happens
        // on destruction so multiple writes coalesce into one signal.
        EXPECT_EQ(*model.at(0), QStringLiteral("b"));
        EXPECT_EQ(changedSpy.count(), 0);
        const QString &pending = *ref;
        EXPECT_EQ(pending, QStringLiteral("b"));
    }

    EXPECT_EQ(*model.at(0), QStringLiteral("b"));
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, GetMutableCommitsLastWrite)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto ref = model.getMutable(0);
        ref = QStringLiteral("b");
        ref = QStringLiteral("c");
        ref = QStringLiteral("d");
    }

    EXPECT_EQ(*model.at(0), QStringLiteral("d"));
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, GetMutableIsNoOpWhenRowErased)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto ref = model.getMutable(0);
        ref = QStringLiteral("new");
        // The erase invalidates the persistent index (and shifts the row out
        // from under the held reference). The destructor sees an invalid
        // index and skips the dataChanged emit.
        model.erase(0);
    }

    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("b"));
    EXPECT_EQ(changedSpy.count(), 0);
}

TEST_F(BasicListModelTest, GetMutableAlwaysEmitsOnDestruction)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto ref = model.getMutable(0);
        // No modification — the destructor still emits dataChanged
        // unconditionally; there is no dirty tracking on the proxy.
    }

    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, GetMutableSupportsInPlaceMutation)
{
    model.push_back(QStringLiteral("hello"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    {
        auto ref = model.getMutable(0);
        ref->append(QStringLiteral(" world"));
    }

    EXPECT_EQ(*model.at(0), QStringLiteral("hello world"));
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, OperatorSubscriptReads)
{
    model.push_back(QStringLiteral("hello"));

    auto ref = model[0];

    EXPECT_EQ(*ref, QStringLiteral("hello"));
    EXPECT_EQ(ref->size(), 5);
}

TEST_F(BasicListModelTest, SetDataDrivesRefValueChanged)
{
    model.push_back(QStringLiteral("a"));

    Ref<QString> *ref = getAsRefObject<Ref<QString>>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);
    QSignalSpy valueChangedSpy(ref, &cute::RefBase::valueChanged);

    model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole);

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), QStringLiteral("b"));
}

TEST_F(BasicListModelTest, EraseEmitsRowsRemoved)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));
    model.push_back(QStringLiteral("c"));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.erase(1);

    ASSERT_EQ(removedSpy.count(), 1);
    const QList<QVariant> args = removedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.size(), 2);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("c"));
}

TEST_F(BasicListModelTest, EraseRangeRemovesInclusive)
{
    for (const char *v : {"a", "b", "c", "d"})
        model.push_back(QString::fromUtf8(v));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.erase(1, 2);

    ASSERT_EQ(removedSpy.count(), 1);
    const QList<QVariant> args = removedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.size(), 2);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("d"));
}

TEST_F(BasicListModelTest, ClearResetsModel)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.clear();

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.size(), 0);
}

TEST_F(BasicListModelTest, ResetReplacesContents)
{
    model.push_back(QStringLiteral("old1"));
    model.push_back(QStringLiteral("old2"));

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.reset({QStringLiteral("new1"),
                 QStringLiteral("new2"),
                 QStringLiteral("new3")});

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(*model.at(0), QStringLiteral("new1"));
    EXPECT_EQ(*model.at(1), QStringLiteral("new2"));
    EXPECT_EQ(*model.at(2), QStringLiteral("new3"));
}

TEST_F(BasicListModelTest, ResetWithEmptyClears)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.reset({});

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_TRUE(model.empty());
}

TEST_F(BasicListModelTest, ResizeGrowsAndShrinks)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.resize(3);
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.size(), 3);

    model.resize(1);
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
}

TEST_F(BasicListModelTest, ResizeWithDefaultValueFillsNewRows)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.resize(4, QStringLiteral("fill"));

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 3);
    EXPECT_EQ(model.size(), 4);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
    EXPECT_EQ(*model.at(1), QStringLiteral("fill"));
    EXPECT_EQ(*model.at(2), QStringLiteral("fill"));
    EXPECT_EQ(*model.at(3), QStringLiteral("fill"));
}

TEST_F(BasicListModelTest, ResizeShrinkIgnoresDefaultValue)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));
    model.push_back(QStringLiteral("c"));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.resize(1, QStringLiteral("unused"));

    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("a"));
}

TEST_F(BasicListModelTest, ResizeToSameSizeIsNoOp)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.resize(2, QStringLiteral("fill"));

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(removedSpy.count(), 0);
    EXPECT_EQ(model.size(), 2);
}

TEST_F(BasicListModelTest, ConstIterationVisitsAllInOrder)
{
    const QStringList expected = {QStringLiteral("a"),
                                  QStringLiteral("b"),
                                  QStringLiteral("c")};
    for (const QString &s : expected)
        model.push_back(s);

    QStringList seen;
    for (const QString &s : model)
        seen.push_back(s);

    EXPECT_EQ(seen, expected);
}

TEST_F(PodListModelTest, RefGetValueReadsPodFromValueRole)
{
    model.push_back(Pod{7, QStringLiteral("seven")});

    Ref<Pod> ref(QPersistentModelIndex(model.index(0, 0)));

    EXPECT_TRUE(ref.getValue() == (Pod{7, QStringLiteral("seven")}));
}

TEST_F(PodListModelTest, GetMutableDrivesRefValueChanged)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    Ref<Pod> ref(QPersistentModelIndex(model.index(0, 0)));
    QSignalSpy valueChangedSpy(&ref, &cute::RefBase::valueChanged);

    model.getMutable(0) = Pod{2, QStringLiteral("two")};

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_TRUE(ref.getValue() == (Pod{2, QStringLiteral("two")}));
}

TEST_F(PodListModelTest, SetDataDrivesRefValueChanged)
{
    model.push_back(Pod{1, QStringLiteral("one")});

    Ref<Pod> *ref = getAsRefObject<Ref<Pod>>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);
    QSignalSpy valueChangedSpy(ref, &cute::RefBase::valueChanged);

    EXPECT_TRUE(model.setData(model.index(0, 0),
                              QVariant::fromValue(Pod{42, QStringLiteral("answer")}),
                              ValueRole));

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_TRUE(ref->getValue() == (Pod{42, QStringLiteral("answer")}));
}

// ---------- setData strict-role contract ----------

TEST_F(BasicListModelTest, ValueRoleWritesStraightToStorage)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole));

    EXPECT_EQ(*model.at(0), QStringLiteral("b"));
    EXPECT_EQ(rawStorageSnapshot(model), QStringList{QStringLiteral("b")});
    EXPECT_EQ(changedSpy.count(), 1);
}

TEST_F(BasicListModelTest, DataChangedEmitsEmptyRolesList)
{
    model.push_back(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole);

    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    ASSERT_GE(args.size(), 3);
    EXPECT_TRUE(args.at(2).value<QList<int>>().isEmpty());
}

TEST_F(BasicListModelTest, SetDataWithUnsupportedRoleTerminates)
{
    model.push_back(QStringLiteral("a"));

    EXPECT_DEATH(
        { (void)model.setData(model.index(0, 0), QStringLiteral("x"), Qt::DisplayRole); },
        "");
    EXPECT_DEATH(
        { (void)model.setData(model.index(0, 0), QStringLiteral("x"), Qt::EditRole); }, "");
    EXPECT_DEATH(
        { (void)model.setData(model.index(0, 0), QStringLiteral("x"), Qt::UserRole + 5); },
        "");
}

// ---------- Drag / drop ----------

TEST_F(DroppingListModelTest, MimeDataForSingleIndexReadsValue)
{
    model.push_back(QStringLiteral("hello"));

    QMimeData *mime = model.mimeData({model.index(0, 0)});
    ASSERT_NE(mime, nullptr);
    EXPECT_EQ(mime->text(), QStringLiteral("hello"));
    delete mime;
}

TEST_F(DroppingListModelTest, MimeDataReadsCurrentStorageValue)
{
    model.push_back(QStringLiteral("a"));
    model.setData(model.index(0, 0), QStringLiteral("updated"), ValueRole);

    QMimeData *mime = model.mimeData({model.index(0, 0)});
    ASSERT_NE(mime, nullptr);
    EXPECT_EQ(mime->text(), QStringLiteral("updated"));
    delete mime;
}

TEST_F(DroppingListModelTest, MimeDataForMultipleIndexesReturnsNull)
{
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    EXPECT_EQ(model.mimeData({model.index(0, 0), model.index(1, 0)}), nullptr);
}

TEST_F(DroppingListModelTest, DropAtRootBetweenRowsPassesNulloptParent)
{
    QMimeData payload;
    payload.setText(QStringLiteral("incoming"));

    EXPECT_TRUE(model.canDropMimeData(&payload, Qt::CopyAction, 0, 0, QModelIndex()));
    EXPECT_FALSE(model.lastCanDropParent.has_value());
    ASSERT_TRUE(model.lastCanDropRow.has_value());
    EXPECT_EQ(*model.lastCanDropRow, 0);
    EXPECT_EQ(model.lastCanDropColumn, 0);
    EXPECT_EQ(model.lastCanDropAction, Qt::CopyAction);
    ASSERT_TRUE(model.lastCanDropText.has_value());
    EXPECT_EQ(*model.lastCanDropText, QStringLiteral("incoming"));

    EXPECT_TRUE(model.dropMimeData(&payload, Qt::CopyAction, 0, 0, QModelIndex()));
    EXPECT_FALSE(model.lastDropParent.has_value());
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(*model.at(0), QStringLiteral("incoming"));
}

TEST_F(DroppingListModelTest, DropOnExistingItemPassesNulloptRow)
{
    model.push_back(QStringLiteral("parent"));

    QMimeData payload;
    payload.setText(QStringLiteral("payload"));

    EXPECT_TRUE(model.dropMimeData(&payload, Qt::MoveAction, -1, 0, model.index(0, 0)));

    ASSERT_TRUE(model.lastDropParent.has_value());
    EXPECT_EQ(*model.lastDropParent, QStringLiteral("parent"));
    EXPECT_FALSE(model.lastDropRow.has_value());
    EXPECT_EQ(model.lastDropAction, Qt::MoveAction);
}

TEST_F(DroppingListModelTest, BaseModelDefaultDropRejectsAll)
{
    StringListModel plain;
    plain.push_back(QStringLiteral("a"));
    QMimeData payload;
    payload.setText(QStringLiteral("ignored"));

    EXPECT_FALSE(plain.canDropMimeData(&payload, Qt::CopyAction, 0, 0, QModelIndex()));
    EXPECT_FALSE(plain.dropMimeData(&payload, Qt::CopyAction, 0, 0, QModelIndex()));
    EXPECT_EQ(plain.size(), 1);
}
