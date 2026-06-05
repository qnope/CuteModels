#include "CuteModel/BaseModel.h"
#include "CuteModel/BasicListModel.h"
#include "CuteModel/ModelTraversal.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>

#include <memory>
#include <vector>

using cute::BaseModel;
using cute::BasicListModel;

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

// Arbre minimal à pointeurs parents stables, pour exercer la récursion.
struct TreeNode
{
    int value = 0;
    TreeNode *parent = nullptr;
    std::vector<std::unique_ptr<TreeNode>> children;
};

class IntTreeModel : public BaseModel<int>
{
public:
    TreeNode *addNode(TreeNode *parent, int value)
    {
        TreeNode *p = parent ? parent : &m_root;
        auto node = std::make_unique<TreeNode>();
        node->value = value;
        node->parent = p;
        TreeNode *raw = node.get();
        p->children.push_back(std::move(node));
        return raw;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return static_cast<int>(nodeFor(parent)->children.size());
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (column != 0)
            return {};
        const TreeNode *p = nodeFor(parent);
        if (row < 0 || row >= static_cast<int>(p->children.size()))
            return {};
        return createIndex(row, column, p->children[row].get());
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid())
            return {};
        TreeNode *node = static_cast<TreeNode *>(child.internalPointer());
        TreeNode *p = node->parent;
        if (!p || p == &m_root)
            return {};
        return createIndex(rowOf(p), 0, p);
    }

    QVariant data(const int &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value;
        return {};
    }

    const int &getStorageValue(const QModelIndex &index) const override
    {
        return static_cast<TreeNode *>(index.internalPointer())->value;
    }

    void setStorageValue(const QModelIndex &index, int value) override
    {
        static_cast<TreeNode *>(index.internalPointer())->value = value;
        emit dataChanged(index, index);
    }

private:
    const TreeNode *nodeFor(const QModelIndex &index) const
    {
        return index.isValid()
                   ? static_cast<const TreeNode *>(index.internalPointer())
                   : &m_root;
    }

    int rowOf(TreeNode *node) const
    {
        const auto &siblings = node->parent->children;
        for (int i = 0; i < static_cast<int>(siblings.size()); ++i)
            if (siblings[i].get() == node)
                return i;
        return 0;
    }

    TreeNode m_root;
};

std::vector<int> valuesAt(const BaseModel<int> &model,
                          const std::vector<QModelIndex> &indexes)
{
    std::vector<int> values;
    for (const QModelIndex &index : indexes)
        values.push_back(model.getStorageValue(index));
    return values;
}

}

TEST(ModelTraversalTest, ForEachIndexVisitsListInOrder)
{
    IntListModel model;
    model.push_back(10);
    model.push_back(20);
    model.push_back(30);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    std::vector<int> values;
    std::vector<int> rows;
    cute::forEachIndex<int>(model, [&](const int &value, const QModelIndex &index) {
        values.push_back(value);
        rows.push_back(index.row());
    });

    EXPECT_EQ(values, (std::vector<int>{10, 20, 30}));
    EXPECT_EQ(rows, (std::vector<int>{0, 1, 2}));
}

TEST(ModelTraversalTest, IndexesMatchingFiltersByPredicate)
{
    IntListModel model;
    for (int value : {10, 20, 30, 40})
        model.push_back(value);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    const auto matches = cute::indexesMatching<int>(
        model, [](const int &value) { return value % 20 == 0; });

    ASSERT_EQ(matches.size(), 2u);
    EXPECT_EQ(matches[0].row(), 1);
    EXPECT_EQ(matches[1].row(), 3);
    EXPECT_EQ(valuesAt(model, matches), (std::vector<int>{20, 40}));
}

TEST(ModelTraversalTest, ColumnPolicyControlsVisitedColumns)
{
    IntListModel model(QStringList{QStringLiteral("a"), QStringLiteral("b")});
    model.push_back(10);
    model.push_back(20);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    std::vector<int> allColumns;
    cute::forEachIndex<int>(
        model,
        [&](const int &value, const QModelIndex &) { allColumns.push_back(value); },
        cute::ColumnPolicy::AllColumns);
    EXPECT_EQ(allColumns, (std::vector<int>{10, 10, 20, 20}));

    std::vector<int> firstColumn;
    cute::forEachIndex<int>(
        model,
        [&](const int &value, const QModelIndex &) { firstColumn.push_back(value); },
        cute::ColumnPolicy::FirstColumnOnly);
    EXPECT_EQ(firstColumn, (std::vector<int>{10, 20}));
}

TEST(ModelTraversalTest, IndexesMatchingHonoursColumnPolicy)
{
    IntListModel model(QStringList{QStringLiteral("a"), QStringLiteral("b")});
    model.push_back(10);
    model.push_back(20);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    const auto allColumns = cute::indexesMatching<int>(
        model, [](const int &value) { return value == 20; },
        cute::ColumnPolicy::AllColumns);
    EXPECT_EQ(allColumns.size(), 2u);

    const auto firstColumn = cute::indexesMatching<int>(
        model, [](const int &value) { return value == 20; },
        cute::ColumnPolicy::FirstColumnOnly);
    ASSERT_EQ(firstColumn.size(), 1u);
    EXPECT_EQ(firstColumn[0].row(), 1);
    EXPECT_EQ(firstColumn[0].column(), 0);
}

TEST(ModelTraversalTest, IndexesMatchingEmptyWhenNoneMatch)
{
    IntListModel model;
    model.push_back(1);
    model.push_back(2);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    const auto matches = cute::indexesMatching<int>(
        model, [](const int &value) { return value > 100; });

    EXPECT_TRUE(matches.empty());
}

TEST(ModelTraversalTest, ForEachIndexRecursesIntoChildrenDepthFirst)
{
    IntTreeModel model;
    TreeNode *a = model.addNode(nullptr, 1);
    TreeNode *b = model.addNode(nullptr, 2);
    model.addNode(a, 3);
    model.addNode(a, 4);
    model.addNode(b, 5);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    std::vector<int> values;
    cute::forEachIndex<int>(model, [&](const int &value, const QModelIndex &) {
        values.push_back(value);
    });

    EXPECT_EQ(values, (std::vector<int>{1, 3, 4, 2, 5}));
}

TEST(ModelTraversalTest, IndexesMatchingWalksWholeTree)
{
    IntTreeModel model;
    TreeNode *a = model.addNode(nullptr, 1);
    TreeNode *b = model.addNode(nullptr, 2);
    model.addNode(a, 3);
    model.addNode(a, 4);
    model.addNode(b, 5);
    QAbstractItemModelTester tester(&model, Reporting::Fatal);

    const auto even = cute::indexesMatching<int>(
        model, [](const int &value) { return value % 2 == 0; });

    EXPECT_EQ(valuesAt(model, even), (std::vector<int>{4, 2}));
}
