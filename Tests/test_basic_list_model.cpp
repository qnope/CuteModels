#include "CuteModel/BasicListModel.h"
#include "CuteModel/GetAsRefObject.h"
#include "CuteModel/Ref.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QModelIndex>
#include <QModelRoleData>
#include <QObject>
#include <QPersistentModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>

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

    QVariant data(const QString &value, int role) const override
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

class PodListModel : public BasicListModel<Pod>
{
public:
    using BasicListModel<Pod>::BasicListModel;
    using BasicListModel<Pod>::data;

    QVariant data(const Pod &value, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value.label;
        return {};
    }
};

} // namespace

TEST(BasicListModel, RowCountReflectsStorage)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_TRUE(model.empty());

    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));
    model.push_back(QStringLiteral("c"));

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(model.rowCount(model.index(0, 0)), 0);
}

TEST(BasicListModel, ValueRoleRoundTripFromBase)
{
    StringListModel strings;
    QAbstractItemModelTester stringsTester(&strings, QAbstractItemModelTester::FailureReportingMode::Fatal);
    strings.push_back(QStringLiteral("hello"));
    EXPECT_EQ(strings.index(0, 0).data(ValueRole).value<QString>(),
              QStringLiteral("hello"));

    PodListModel pods;
    QAbstractItemModelTester podsTester(&pods, QAbstractItemModelTester::FailureReportingMode::Fatal);
    pods.push_back(Pod{7, QStringLiteral("seven")});
    const QVariant valueVariant = pods.index(0, 0).data(ValueRole);
    ASSERT_TRUE(valueVariant.isValid());
    EXPECT_TRUE(valueVariant.value<Pod>() == (Pod{7, QStringLiteral("seven")}));
}

TEST(BasicListModel, ProjectionDrivesDisplayRole)
{
    PodListModel pods;
    QAbstractItemModelTester podsTester(&pods, QAbstractItemModelTester::FailureReportingMode::Fatal);
    pods.push_back(Pod{1, QStringLiteral("one")});

    EXPECT_EQ(pods.index(0, 0).data(Qt::DisplayRole).toString(),
              QStringLiteral("one"));
    EXPECT_FALSE(pods.index(0, 0).data(9999).isValid());
}

TEST(BasicListModel, MultiDataFillsHandledRoles)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
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

TEST(BasicListModel, PushBackEmitsRowsInserted)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    int inserts = 0;
    int first = -1;
    int last = -1;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                     [&](const QModelIndex &, int f, int l) {
                         ++inserts;
                         first = f;
                         last = l;
                     });

    model.push_back(QStringLiteral("a"));

    EXPECT_EQ(inserts, 1);
    EXPECT_EQ(first, 0);
    EXPECT_EQ(last, 0);
}

TEST(BasicListModel, EmplaceBackConstructsInPlace)
{
    PodListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    int inserts = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                     [&](const QModelIndex &, int, int) { ++inserts; });

    Pod &ref = model.emplace_back(Pod{42, QStringLiteral("answer")});

    EXPECT_EQ(ref.id, 42);
    EXPECT_EQ(inserts, 1);
    EXPECT_EQ(model.at(0).label, QStringLiteral("answer"));
}

TEST(BasicListModel, SetDataEmitsDataChangedAndUpdatesValue)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));

    int changes = 0;
    QModelIndex tl;
    QModelIndex br;
    QObject::connect(&model, &QAbstractItemModel::dataChanged,
                     [&](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                         ++changes;
                         tl = topLeft;
                         br = bottomRight;
                     });

    EXPECT_TRUE(model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole));
    EXPECT_EQ(model.at(0), QStringLiteral("b"));
    EXPECT_EQ(changes, 1);
    EXPECT_EQ(tl, model.index(0, 0));
    EXPECT_EQ(br, model.index(0, 0));

    PodListModel pods;
    QAbstractItemModelTester podsTester(&pods, QAbstractItemModelTester::FailureReportingMode::Fatal);
    pods.push_back(Pod{1, QStringLiteral("one")});
    int podChanges = 0;
    QObject::connect(&pods, &QAbstractItemModel::dataChanged,
                     [&](const QModelIndex &, const QModelIndex &) { ++podChanges; });

    EXPECT_FALSE(pods.setData(pods.index(0, 0), QStringLiteral("nope"), ValueRole));
    EXPECT_EQ(podChanges, 0);
}

TEST(BasicListModel, OperatorSubscriptWriteEmitsDataChanged)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));

    int changes = 0;
    QObject::connect(&model, &QAbstractItemModel::dataChanged,
                     [&](const QModelIndex &, const QModelIndex &) { ++changes; });

    model[0] = QStringLiteral("z");
    EXPECT_EQ(model.at(0), QStringLiteral("z"));
    EXPECT_EQ(changes, 1);

    QString read = model[0];
    EXPECT_EQ(read, QStringLiteral("z"));
    EXPECT_EQ(changes, 1);
}

TEST(BasicListModel, OperatorSubscriptDrivesRefValueChanged)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));

    Ref<QString> ref(QPersistentModelIndex(model.index(0, 0)));
    int changes = 0;
    QObject::connect(&ref, &cute::RefBase::valueChanged, [&] { ++changes; });

    model[0] = QStringLiteral("new");

    EXPECT_EQ(changes, 1);
    EXPECT_EQ(ref.getValue(), QStringLiteral("new"));
}

TEST(BasicListModel, SetDataDrivesRefValueChanged)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));

    Ref<QString> *ref = getAsRefObject<Ref<QString>>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);
    int changes = 0;
    QObject::connect(ref, &cute::RefBase::valueChanged, [&] { ++changes; });

    model.setData(model.index(0, 0), QStringLiteral("b"), ValueRole);

    EXPECT_EQ(changes, 1);
    EXPECT_EQ(ref->getValue(), QStringLiteral("b"));

    delete ref;
}

TEST(BasicListModel, EraseEmitsRowsRemoved)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));
    model.push_back(QStringLiteral("c"));

    int removes = 0;
    int first = -1;
    int last = -1;
    QObject::connect(&model, &QAbstractItemModel::rowsRemoved,
                     [&](const QModelIndex &, int f, int l) {
                         ++removes;
                         first = f;
                         last = l;
                     });

    model.erase(1);

    EXPECT_EQ(removes, 1);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(last, 1);
    EXPECT_EQ(model.size(), 2);
    EXPECT_EQ(model.at(0), QStringLiteral("a"));
    EXPECT_EQ(model.at(1), QStringLiteral("c"));
}

TEST(BasicListModel, EraseRangeRemovesInclusive)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    for (const char *v : {"a", "b", "c", "d"})
        model.push_back(QString::fromUtf8(v));

    model.erase(1, 2);

    EXPECT_EQ(model.size(), 2);
    EXPECT_EQ(model.at(0), QStringLiteral("a"));
    EXPECT_EQ(model.at(1), QStringLiteral("d"));
}

TEST(BasicListModel, InsertShiftsRows)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("c"));

    int inserts = 0;
    int first = -1;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                     [&](const QModelIndex &, int f, int) {
                         ++inserts;
                         first = f;
                     });

    model.insert(1, QStringLiteral("b"));

    EXPECT_EQ(inserts, 1);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(model.size(), 3);
    EXPECT_EQ(model.at(0), QStringLiteral("a"));
    EXPECT_EQ(model.at(1), QStringLiteral("b"));
    EXPECT_EQ(model.at(2), QStringLiteral("c"));
}

TEST(BasicListModel, ClearResetsModel)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));
    model.push_back(QStringLiteral("b"));

    int resets = 0;
    QObject::connect(&model, &QAbstractItemModel::modelReset, [&] { ++resets; });

    model.clear();

    EXPECT_EQ(resets, 1);
    EXPECT_EQ(model.size(), 0);
}

TEST(BasicListModel, ResizeGrowsAndShrinks)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.push_back(QStringLiteral("a"));

    int inserts = 0;
    int removes = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                     [&](const QModelIndex &, int, int) { ++inserts; });
    QObject::connect(&model, &QAbstractItemModel::rowsRemoved,
                     [&](const QModelIndex &, int, int) { ++removes; });

    model.resize(3);
    EXPECT_EQ(inserts, 1);
    EXPECT_EQ(model.size(), 3);

    model.resize(1);
    EXPECT_EQ(removes, 1);
    EXPECT_EQ(model.size(), 1);
    EXPECT_EQ(model.at(0), QStringLiteral("a"));
}

TEST(BasicListModel, ConstIterationVisitsAllInOrder)
{
    StringListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
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
