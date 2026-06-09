#include "CuteModel/ModelTraversal.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/TreeModel.h"
#include "CuteModel/TreeNode.h"
#include "CuteModel/ValueRole.h"
#include "common/NonDefault.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QMimeData>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <memory>
#include <optional>
#include <vector>

using cute::TreeModel;
using cute::TreeNode;
using cute::ValueRole;

namespace {

using Reporting = QAbstractItemModelTester::FailureReportingMode;

class StringTreeModel : public TreeModel<std::shared_ptr<TreeNode<QString>>>
{
public:
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::TreeModel;
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::data;

    QVariant data(const std::shared_ptr<TreeNode<QString>> &node, const QModelIndex &index, int role) const override
    {
        if (index.column() != 0)
            return {};
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return node->payload();
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

class MultiColumnTreeModel : public TreeModel<std::shared_ptr<TreeNode<Pod>>>
{
public:
    using TreeModel<std::shared_ptr<TreeNode<Pod>>>::TreeModel;
    using TreeModel<std::shared_ptr<TreeNode<Pod>>>::data;

    QVariant data(const std::shared_ptr<TreeNode<Pod>> &node, const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        if (index.column() == 0)
            return node->payload().label;
        if (index.column() == 1)
            return node->payload().id;
        return {};
    }
};

using cute_tests::NonDefault;

class NonDefaultTreeModel : public TreeModel<std::shared_ptr<TreeNode<NonDefault>>>
{
public:
    using TreeModel<std::shared_ptr<TreeNode<NonDefault>>>::TreeModel;
    using TreeModel<std::shared_ptr<TreeNode<NonDefault>>>::data;

    QVariant data(const std::shared_ptr<TreeNode<NonDefault>> &node, const QModelIndex &index, int role) const override
    {
        if (index.column() != 0)
            return {};
        if (role == Qt::DisplayRole)
            return node->payload().value;
        return {};
    }
};

class DroppingTreeModel : public TreeModel<std::shared_ptr<TreeNode<QString>>>
{
public:
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::TreeModel;
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::data;

    QVariant data(const std::shared_ptr<TreeNode<QString>> &node, const QModelIndex &index, int role) const override
    {
        if (index.column() != 0)
            return {};
        if (role == Qt::DisplayRole)
            return node->payload();
        return {};
    }

    bool canDropOnElement(const std::shared_ptr<TreeNode<QString>> &node, const QModelIndex &index,
                          Qt::DropAction action, const QMimeData *data) const override
    {
        lastDropOnElement = node->payload();
        lastDropOnIndex = index;
        lastDropOnAction = action;
        lastDropOnText = data ? std::optional<QString>(data->text()) : std::nullopt;
        return data && data->hasText();
    }

    mutable std::optional<QString> lastDropOnElement;
    mutable QPersistentModelIndex lastDropOnIndex;
    mutable Qt::DropAction lastDropOnAction = Qt::IgnoreAction;
    mutable std::optional<QString> lastDropOnText;
};

class ExposedStringTreeModel : public StringTreeModel
{
public:
    using StringTreeModel::StringTreeModel;
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::setStorageValue;
    using TreeModel<std::shared_ptr<TreeNode<QString>>>::getStorageValue;
};

class StringTreeModelTest : public ::testing::Test
{
protected:
    StringTreeModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
};

class ExposedStringTreeModelTest : public ::testing::Test
{
protected:
    ExposedStringTreeModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
};

class MultiColumnTreeModelTest : public ::testing::Test
{
protected:
    MultiColumnTreeModel model{QStringList{QStringLiteral("label"), QStringLiteral("id")}};
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
};

std::shared_ptr<TreeNode<QString>> makeStringNode(const QString &payload)
{
    return std::make_shared<TreeNode<QString>>(payload);
}

}

TEST_F(StringTreeModelTest, EmptyTreeIsRootOnly)
{
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 1);
    EXPECT_FALSE(model.hasChildren());
    EXPECT_NE(model.root(), nullptr);
    EXPECT_EQ(model.root()->childCount(), 0);
    EXPECT_EQ(model.root()->parent(), nullptr);
    EXPECT_EQ(model.root()->indexInParent(), -1);
    EXPECT_FALSE(model.root()->modelIndex().isValid());
}

TEST_F(StringTreeModelTest, FreshNodeHasNoModelUntilAttached)
{
    auto fresh = makeStringNode(QStringLiteral("a"));
    EXPECT_EQ(fresh->model(), nullptr);
    EXPECT_EQ(fresh->parent(), nullptr);
    EXPECT_EQ(fresh->indexInParent(), -1);
    EXPECT_FALSE(fresh->modelIndex().isValid());
    EXPECT_EQ(fresh->payload(), QStringLiteral("a"));

    model.root()->push_back_child(fresh);
    EXPECT_EQ(fresh->model(), &model);
    EXPECT_EQ(fresh->indexInParent(), 0);
    EXPECT_TRUE(fresh->modelIndex().isValid());
}

TEST_F(StringTreeModelTest, PushBackChildEmitsRowsInsertedAndUpdatesStructure)
{
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    auto child = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(child);

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_FALSE(args.at(0).value<QModelIndex>().isValid());
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 0);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(child->indexInParent(), 0);
    EXPECT_EQ(child->model(), &model);
    EXPECT_EQ(child->payload(), QStringLiteral("a"));
    EXPECT_TRUE(child->modelIndex(0).isValid());
    EXPECT_FALSE(model.parent(child->modelIndex(0)).isValid());
}

TEST_F(StringTreeModelTest, PrebuiltTwoLevelSubtreeAttachesWithSingleSignal)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    auto inner = makeStringNode(QStringLiteral("inner"));
    outer->push_back_child(inner);

    EXPECT_EQ(outer->model(), nullptr);
    EXPECT_EQ(inner->model(), nullptr);
    EXPECT_EQ(outer->childCount(), 1);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.root()->push_back_child(outer);

    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(outer->model(), &model);
    EXPECT_EQ(inner->model(), &model);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.rowCount(outer->modelIndex(0)), 1);
    EXPECT_EQ(model.parent(inner->modelIndex(0)), outer->modelIndex(0));
    EXPECT_EQ(inner->indexInParent(), 0);
    EXPECT_EQ(outer->indexInParent(), 0);
}

TEST_F(StringTreeModelTest, InsertChildInMiddleRenumbersSiblings)
{
    auto a = makeStringNode(QStringLiteral("a"));
    auto c = makeStringNode(QStringLiteral("c"));
    model.root()->push_back_child(a);
    model.root()->push_back_child(c);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    auto b = makeStringNode(QStringLiteral("b"));
    model.root()->insert_child(1, b);

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(a->indexInParent(), 0);
    EXPECT_EQ(b->indexInParent(), 1);
    EXPECT_EQ(c->indexInParent(), 2);
    EXPECT_EQ(c->modelIndex(0).row(), 2);
}

TEST_F(StringTreeModelTest, AppendRangeEmitsSingleSignalAndAttachesAll)
{
    std::vector<std::shared_ptr<TreeNode<QString>>> batch;
    batch.push_back(makeStringNode(QStringLiteral("a")));
    batch.push_back(makeStringNode(QStringLiteral("b")));
    batch.push_back(makeStringNode(QStringLiteral("c")));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.root()->append_range(batch);

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.root()->child(0)->payload(), QStringLiteral("a"));
    EXPECT_EQ(model.root()->child(2)->payload(), QStringLiteral("c"));
    EXPECT_EQ(model.root()->child(1)->indexInParent(), 1);
}

TEST_F(StringTreeModelTest, InsertRangeInMiddleRenumbers)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));
    auto tail = makeStringNode(QStringLiteral("z"));
    model.root()->push_back_child(tail);

    std::vector<std::shared_ptr<TreeNode<QString>>> batch;
    batch.push_back(makeStringNode(QStringLiteral("b")));
    batch.push_back(makeStringNode(QStringLiteral("c")));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.root()->insert_range(1, batch);

    ASSERT_EQ(insertedSpy.count(), 1);
    const QList<QVariant> args = insertedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(tail->indexInParent(), 3);
    EXPECT_EQ(tail->modelIndex(0).row(), 3);
}

TEST_F(StringTreeModelTest, EmptyRangeAndOutOfRangeAreNoOps)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.root()->append_range({});
    model.root()->insert_range(-1, {makeStringNode(QStringLiteral("x"))});
    model.root()->insert_range(99, {makeStringNode(QStringLiteral("x"))});

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(StringTreeModelTest, SetPayloadOnAttachedNodeEmitsDataChanged)
{
    auto child = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(child);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    child->setPayload(QStringLiteral("b"));

    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(0, 0));
    EXPECT_EQ(args.at(1).value<QModelIndex>(), model.index(0, 0));
    EXPECT_EQ(child->payload(), QStringLiteral("b"));
}

TEST(TreeNodeDetached, SetPayloadOnFreshNodeIsSilent)
{
    StringTreeModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
    auto fresh = makeStringNode(QStringLiteral("a"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    fresh->setPayload(QStringLiteral("b"));

    EXPECT_EQ(changedSpy.count(), 0);
    EXPECT_EQ(fresh->payload(), QStringLiteral("b"));
    EXPECT_EQ(fresh->model(), nullptr);
}

TEST_F(StringTreeModelTest, EraseChildEmitsRowsRemovedAndDetachesSubtree)
{
    auto a = makeStringNode(QStringLiteral("a"));
    auto b = makeStringNode(QStringLiteral("b"));
    auto c = makeStringNode(QStringLiteral("c"));
    auto bChild = makeStringNode(QStringLiteral("b-child"));
    b->push_back_child(bChild);
    model.root()->push_back_child(a);
    model.root()->push_back_child(b);
    model.root()->push_back_child(c);

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    model.root()->erase_child(1);

    ASSERT_EQ(removedSpy.count(), 1);
    const QList<QVariant> args = removedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 1);
    EXPECT_EQ(args.at(2).toInt(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(c->indexInParent(), 1);
    EXPECT_EQ(c->modelIndex(0).row(), 1);

    EXPECT_EQ(b->model(), nullptr);
    EXPECT_EQ(bChild->model(), nullptr);
    EXPECT_FALSE(b->modelIndex(0).isValid());

    QSignalSpy detachedChangedSpy(&model, &QAbstractItemModel::dataChanged);
    b->setPayload(QStringLiteral("orphan"));
    EXPECT_EQ(detachedChangedSpy.count(), 0);
    EXPECT_EQ(b->payload(), QStringLiteral("orphan"));
}

TEST_F(StringTreeModelTest, DetachedSubtreePreservesInternalParentLinks)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    auto inner = makeStringNode(QStringLiteral("inner"));
    auto leaf = makeStringNode(QStringLiteral("leaf"));
    inner->push_back_child(leaf);
    outer->push_back_child(inner);
    model.root()->push_back_child(outer);

    model.root()->erase_child(0);

    EXPECT_EQ(outer->model(), nullptr);
    EXPECT_EQ(inner->model(), nullptr);
    EXPECT_EQ(leaf->model(), nullptr);

    EXPECT_EQ(outer->parent(), nullptr);
    EXPECT_EQ(outer->indexInParent(), -1);

    EXPECT_EQ(inner->parent(), outer);
    EXPECT_EQ(inner->indexInParent(), 0);
    EXPECT_EQ(leaf->parent(), inner);
    EXPECT_EQ(leaf->indexInParent(), 0);

    EXPECT_EQ(outer->childCount(), 1);
    EXPECT_EQ(outer->child(0), inner);
    EXPECT_EQ(inner->child(0), leaf);
}

TEST_F(StringTreeModelTest, EraseChildrenRangeRemovesInclusive)
{
    for (const char *v : {"a", "b", "c", "d"})
        model.root()->push_back_child(makeStringNode(QString::fromUtf8(v)));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    model.root()->erase_children(1, 2);

    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.root()->child(0)->payload(), QStringLiteral("a"));
    EXPECT_EQ(model.root()->child(1)->payload(), QStringLiteral("d"));
    EXPECT_EQ(model.root()->child(1)->indexInParent(), 1);
}

TEST_F(StringTreeModelTest, InsertRowsCreatesDefaultConstructedChildren)
{
    auto a = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(a);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    EXPECT_TRUE(model.insertRows(0, 2));

    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.root()->child(0)->payload(), QString());
    EXPECT_EQ(model.root()->child(1)->payload(), QString());
    EXPECT_EQ(a->indexInParent(), 2);
}

TEST_F(StringTreeModelTest, InsertRowsUnderNonRootParent)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    model.root()->push_back_child(outer);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    EXPECT_TRUE(model.insertRows(0, 2, outer->modelIndex(0)));

    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(insertedSpy.at(0).at(0).value<QModelIndex>(), outer->modelIndex(0));
    EXPECT_EQ(outer->childCount(), 2);
}

TEST_F(StringTreeModelTest, RemoveRowsDelegatesToEraseChildren)
{
    for (const char *v : {"a", "b", "c"})
        model.root()->push_back_child(makeStringNode(QString::fromUtf8(v)));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    EXPECT_TRUE(model.removeRows(0, 2));

    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.root()->child(0)->payload(), QStringLiteral("c"));
}

TEST(TreeModelNonDefault, InsertRowsRejectedButNodeInsertionWorks)
{
    NonDefaultTreeModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    EXPECT_FALSE(model.insertRows(0, 1));
    EXPECT_EQ(insertedSpy.count(), 0);

    auto node = std::make_shared<TreeNode<NonDefault>>(NonDefault{42});
    model.root()->push_back_child(node);
    EXPECT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(model.root()->child(0)->payload().value, 42);
}

TEST_F(MultiColumnTreeModelTest, ProjectionDispatchesPerColumn)
{
    auto node = std::make_shared<TreeNode<Pod>>(Pod{7, QStringLiteral("seven")});
    model.root()->push_back_child(node);

    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toString(), QStringLiteral("seven"));
    EXPECT_EQ(model.index(0, 1).data(Qt::DisplayRole).toInt(), 7);
}

TEST_F(MultiColumnTreeModelTest, HeaderDataReturnsConfiguredLabels)
{
    EXPECT_EQ(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("label"));
    EXPECT_EQ(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("id"));
}

TEST_F(MultiColumnTreeModelTest, SetStorageValueRefreshesEveryColumn)
{
    auto node = std::make_shared<TreeNode<Pod>>(Pod{1, QStringLiteral("one")});
    model.root()->push_back_child(node);

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    node->setPayload(Pod{2, QStringLiteral("two")});

    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(0, 0));
    EXPECT_EQ(args.at(1).value<QModelIndex>(), model.index(0, 1));
}

TEST_F(StringTreeModelTest, ModelTraversalVisitsDepthFirst)
{
    auto a = makeStringNode(QStringLiteral("a"));
    auto b = makeStringNode(QStringLiteral("b"));
    a->push_back_child(makeStringNode(QStringLiteral("a1")));
    a->push_back_child(makeStringNode(QStringLiteral("a2")));
    b->push_back_child(makeStringNode(QStringLiteral("b1")));
    model.root()->push_back_child(a);
    model.root()->push_back_child(b);

    std::vector<QString> seen;
    cute::forEachIndex(
        &model,
        [&](const std::shared_ptr<TreeNode<QString>> &node, const QModelIndex &) {
            seen.push_back(node->payload());
        },
        cute::ColumnPolicy::FirstColumnOnly);

    EXPECT_EQ(seen, (std::vector<QString>{
        QStringLiteral("a"),
        QStringLiteral("a1"),
        QStringLiteral("a2"),
        QStringLiteral("b"),
        QStringLiteral("b1")}));

    const auto matches = cute::indexesMatching(
        &model,
        [](const std::shared_ptr<TreeNode<QString>> &n) { return n->payload().startsWith(QStringLiteral("a")); },
        cute::ColumnPolicy::FirstColumnOnly);
    EXPECT_EQ(matches.size(), 3u);
}

TEST_F(StringTreeModelTest, RefValueChangedFiresOnSetPayload)
{
    auto a = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(a);

    auto ref = model.getRef<>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);
    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);

    a->setPayload(QStringLiteral("b"));

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue()->payload(), QStringLiteral("b"));
}

TEST_F(StringTreeModelTest, RefSignalsHierarchyChangedAndDestroyed)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    auto inner = makeStringNode(QStringLiteral("inner"));
    outer->push_back_child(inner);
    model.root()->push_back_child(outer);

    auto outerRef = model.getRef<>(outer->modelIndex(0));
    ASSERT_NE(outerRef, nullptr);

    QSignalSpy hierarchySpy(outerRef.get(), &cute::RefBase::underlyingHierarchyChanged);
    inner->setPayload(QStringLiteral("inner-updated"));
    EXPECT_EQ(hierarchySpy.count(), 1);

    auto innerRef = model.getRef<>(inner->modelIndex(0));
    ASSERT_NE(innerRef, nullptr);
    QSignalSpy destroyedSpy(innerRef.get(), &cute::RefBase::underlyingValueDestroyed);
    model.root()->erase_child(0);
    EXPECT_EQ(destroyedSpy.count(), 1);
}

TEST_F(StringTreeModelTest, ClearResetsAndPreservesRootIdentity)
{
    auto kept = makeStringNode(QStringLiteral("kept"));
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));
    model.root()->push_back_child(kept);

    const auto rootBefore = model.root();

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.clear();

    EXPECT_EQ(resetSpy.count(), 1);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.root(), rootBefore);
    EXPECT_EQ(kept->model(), nullptr);
    EXPECT_FALSE(kept->modelIndex(0).isValid());
}

TEST(TreeModelDrop, DropOnNonRootIndexRoutesThroughCanDropOnElement)
{
    DroppingTreeModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};

    auto outer = std::make_shared<TreeNode<QString>>(QStringLiteral("outer"));
    auto inner = std::make_shared<TreeNode<QString>>(QStringLiteral("inner"));
    outer->push_back_child(inner);
    model.root()->push_back_child(outer);

    QMimeData payload;
    payload.setText(QStringLiteral("incoming"));

    EXPECT_TRUE(model.canDropMimeData(&payload, Qt::CopyAction, -1, 0, inner->modelIndex(0)));
    ASSERT_TRUE(model.lastDropOnElement.has_value());
    EXPECT_EQ(*model.lastDropOnElement, QStringLiteral("inner"));
    EXPECT_EQ(model.lastDropOnIndex, inner->modelIndex(0));
    EXPECT_EQ(model.lastDropOnAction, Qt::CopyAction);
}

TEST(TreeNodeConstruction, VariadicConstructorForwardsArgsToPayload)
{
    auto node = std::make_shared<TreeNode<QString>>(3, QChar('x'));
    EXPECT_EQ(node->payload(), QString(3, QChar('x')));
}

TEST_F(StringTreeModelTest, ClearChildrenEmitsSingleRowsRemoved)
{
    for (const char *v : {"a", "b", "c"})
        model.root()->push_back_child(makeStringNode(QString::fromUtf8(v)));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    model.root()->clear_children();

    ASSERT_EQ(removedSpy.count(), 1);
    const QList<QVariant> args = removedSpy.at(0);
    EXPECT_EQ(args.at(1).toInt(), 0);
    EXPECT_EQ(args.at(2).toInt(), 2);
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_TRUE(model.root()->empty());
}

TEST_F(StringTreeModelTest, ClearOnEmptyTreeIsNoOp)
{
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.clear();
    EXPECT_EQ(resetSpy.count(), 0);
}

TEST_F(StringTreeModelTest, HasChildrenDistinguishesLeavesAndBranches)
{
    auto branch = makeStringNode(QStringLiteral("branch"));
    branch->push_back_child(makeStringNode(QStringLiteral("leaf-of-branch")));
    auto leaf = makeStringNode(QStringLiteral("leaf"));
    model.root()->push_back_child(branch);
    model.root()->push_back_child(leaf);

    EXPECT_TRUE(model.hasChildren());
    EXPECT_TRUE(model.hasChildren(branch->modelIndex(0)));
    EXPECT_FALSE(model.hasChildren(leaf->modelIndex(0)));
}

TEST_F(StringTreeModelTest, NodeAtMapsInvalidIndexToRootAndValidIndexToNode)
{
    auto child = makeStringNode(QStringLiteral("x"));
    model.root()->push_back_child(child);

    EXPECT_EQ(model.nodeAt(QModelIndex()), model.root());
    EXPECT_EQ(model.nodeAt(child->modelIndex()), child);
}

TEST_F(StringTreeModelTest, InsertChildNullPointerIsNoOp)
{
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.root()->insert_child(0, nullptr);
    model.root()->push_back_child(nullptr);

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(StringTreeModelTest, InsertRowsRejectsInvalidArguments)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    EXPECT_FALSE(model.insertRows(-1, 1));
    EXPECT_FALSE(model.insertRows(99, 1));
    EXPECT_FALSE(model.insertRows(0, -1));

    EXPECT_TRUE(model.insertRows(0, 0));

    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(StringTreeModelTest, RemoveRowsRejectsInvalidArguments)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    EXPECT_FALSE(model.removeRows(-1, 1));
    EXPECT_FALSE(model.removeRows(0, 99));
    EXPECT_FALSE(model.removeRows(0, -1));

    EXPECT_TRUE(model.removeRows(0, 0));

    EXPECT_EQ(removedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(StringTreeModelTest, RemoveRowsUnderNonRootParent)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    outer->push_back_child(makeStringNode(QStringLiteral("a")));
    outer->push_back_child(makeStringNode(QStringLiteral("b")));
    model.root()->push_back_child(outer);

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    EXPECT_TRUE(model.removeRows(0, 1, outer->modelIndex(0)));

    ASSERT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(removedSpy.at(0).at(0).value<QModelIndex>(), outer->modelIndex(0));
    EXPECT_EQ(outer->childCount(), 1);
    EXPECT_EQ(outer->child(0)->payload(), QStringLiteral("b"));
}

TEST_F(StringTreeModelTest, SetPayloadOnRootIsSilent)
{
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    model.root()->setPayload(QStringLiteral("ignored"));
    EXPECT_EQ(changedSpy.count(), 0);
    EXPECT_EQ(model.root()->childCount(), 0);
}

TEST_F(StringTreeModelTest, RefValueIsTheNodeItself)
{
    auto child = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(child);

    auto ref = model.getRef<>(child->modelIndex(0));
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue().get(), child.get());
    EXPECT_EQ(ref->getValue()->payload(), QStringLiteral("a"));
}

TEST_F(StringTreeModelTest, ReattachDetachedSubtreeRestoresFullStructure)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    auto inner = makeStringNode(QStringLiteral("inner"));
    outer->push_back_child(inner);
    model.root()->push_back_child(outer);

    model.root()->erase_child(0);
    ASSERT_EQ(outer->model(), nullptr);
    ASSERT_EQ(inner->model(), nullptr);

    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.root()->push_back_child(outer);

    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(outer->model(), &model);
    EXPECT_EQ(inner->model(), &model);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.rowCount(outer->modelIndex(0)), 1);
    EXPECT_EQ(inner->parent(), outer);

    inner->setPayload(QStringLiteral("inner-updated"));
    EXPECT_EQ(inner->payload(), QStringLiteral("inner-updated"));
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueReplacesNodeAtValidIndex)
{
    auto original = makeStringNode(QStringLiteral("original"));
    model.root()->push_back_child(original);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    const QModelIndex idx = model.index(0, 0);
    model.setStorageValue(idx, replacement);

    EXPECT_EQ(model.root()->child(0), replacement);
    EXPECT_EQ(replacement->model(), &model);
    EXPECT_EQ(replacement->parent(), model.root());
    EXPECT_EQ(replacement->indexInParent(), 0);
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toString(),
              QStringLiteral("replacement"));
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueDetachesOldNode)
{
    auto original = makeStringNode(QStringLiteral("original"));
    auto originalChild = makeStringNode(QStringLiteral("original-child"));
    original->push_back_child(originalChild);
    model.root()->push_back_child(original);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    model.setStorageValue(model.index(0, 0), replacement);

    EXPECT_EQ(original->model(), nullptr);
    EXPECT_EQ(originalChild->model(), nullptr);
    EXPECT_EQ(original->parent(), nullptr);
    EXPECT_EQ(original->indexInParent(), -1);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueEmitsRemoveThenInsert)
{
    auto original = makeStringNode(QStringLiteral("original"));
    model.root()->push_back_child(original);

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    model.setStorageValue(model.index(0, 0), replacement);

    ASSERT_EQ(removedSpy.count(), 1);
    ASSERT_EQ(insertedSpy.count(), 1);
    EXPECT_EQ(removedSpy.at(0).at(1).toInt(), 0);
    EXPECT_EQ(removedSpy.at(0).at(2).toInt(), 0);
    EXPECT_EQ(insertedSpy.at(0).at(1).toInt(), 0);
    EXPECT_EQ(insertedSpy.at(0).at(2).toInt(), 0);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueAdoptsReplacementChildren)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("original")));

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    auto replacementChild = makeStringNode(QStringLiteral("replacement-child"));
    replacement->push_back_child(replacementChild);

    model.setStorageValue(model.index(0, 0), replacement);

    EXPECT_EQ(replacement->model(), &model);
    EXPECT_EQ(replacementChild->model(), &model);
    EXPECT_EQ(model.rowCount(model.index(0, 0)), 1);
    EXPECT_EQ(replacementChild->parent(), replacement);
    EXPECT_EQ(replacementChild->indexInParent(), 0);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueOnInvalidIndexIsNoOp)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    model.setStorageValue(QModelIndex{}, replacement);

    EXPECT_EQ(removedSpy.count(), 0);
    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.root()->child(0)->payload(), QStringLiteral("a"));
    EXPECT_EQ(replacement->model(), nullptr);
    EXPECT_EQ(replacement->parent(), nullptr);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueNullValueIsNoOp)
{
    auto original = makeStringNode(QStringLiteral("original"));
    model.root()->push_back_child(original);

    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.setStorageValue(model.index(0, 0), nullptr);

    EXPECT_EQ(removedSpy.count(), 0);
    EXPECT_EQ(insertedSpy.count(), 0);
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.root()->child(0), original);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValuePreservesSiblings)
{
    model.root()->push_back_child(makeStringNode(QStringLiteral("a")));
    auto middle = makeStringNode(QStringLiteral("b"));
    model.root()->push_back_child(middle);
    model.root()->push_back_child(makeStringNode(QStringLiteral("c")));

    auto replacement = makeStringNode(QStringLiteral("B"));
    model.setStorageValue(model.index(1, 0), replacement);

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.root()->child(0)->payload(), QStringLiteral("a"));
    EXPECT_EQ(model.root()->child(1), replacement);
    EXPECT_EQ(model.root()->child(2)->payload(), QStringLiteral("c"));
    EXPECT_EQ(middle->parent(), nullptr);
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueViaRefSetValue)
{
    auto original = makeStringNode(QStringLiteral("original"));
    model.root()->push_back_child(original);

    auto ref = model.getRef<>(model.index(0, 0));
    ASSERT_NE(ref, nullptr);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    ref->setValue(replacement);

    EXPECT_EQ(model.root()->child(0), replacement);
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toString(),
              QStringLiteral("replacement"));
}

TEST_F(ExposedStringTreeModelTest, SetStorageValueUnderNonRootParent)
{
    auto outer = makeStringNode(QStringLiteral("outer"));
    auto inner = makeStringNode(QStringLiteral("inner"));
    outer->push_back_child(inner);
    model.root()->push_back_child(outer);

    auto replacement = makeStringNode(QStringLiteral("replacement"));
    const QModelIndex innerIdx = model.index(0, 0, outer->modelIndex(0));
    model.setStorageValue(innerIdx, replacement);

    EXPECT_EQ(outer->child(0), replacement);
    EXPECT_EQ(replacement->parent(), outer);
    EXPECT_EQ(replacement->model(), &model);
    EXPECT_EQ(inner->parent(), nullptr);
    EXPECT_EQ(inner->model(), nullptr);
}

TEST_F(ExposedStringTreeModelTest, GetStorageValueReturnsSharedPtrAtIndex)
{
    auto child = makeStringNode(QStringLiteral("a"));
    model.root()->push_back_child(child);

    const auto &stored = model.getStorageValue(model.index(0, 0));
    EXPECT_EQ(stored, child);
    EXPECT_EQ(stored.get(), child.get());
}
