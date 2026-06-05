#include "CuteModel/BasicListModel.h"
#include "CuteModel/BasicTableModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/SelectionModel.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QSignalSpy>
#include <QVariant>

#include <algorithm>
#include <optional>
#include <vector>

using cute::BasicListModel;
using cute::BasicTableModel;
using cute::SelectionMode;
using cute::SelectionModel;

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

class IntTableModel : public BasicTableModel<int>
{
public:
    using BasicTableModel<int>::BasicTableModel;
    using BasicTableModel<int>::data;

    QVariant data(const int &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return value;
        return {};
    }
};

std::vector<int> sorted(std::vector<int> values)
{
    std::sort(values.begin(), values.end());
    return values;
}

class SelectionModelListTest : public ::testing::Test
{
protected:
    IntListModel model;
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
    SelectionModel<int> selection{&model, SelectionMode::List};

    void SetUp() override { model.reset({10, 20, 30}); }

    QModelIndex idx(int row) { return model.index(row, 0); }
};

class SelectionModelTableTest : public ::testing::Test
{
protected:
    IntTableModel model{2, 2, 0};
    QAbstractItemModelTester tester{&model, Reporting::Fatal};
    SelectionModel<int> selection{&model, SelectionMode::Table};

    void SetUp() override
    {
        model.getMutable(0, 0) = 1;
        model.getMutable(0, 1) = 2;
        model.getMutable(1, 0) = 3;
        model.getMutable(1, 1) = 4;
    }

    QModelIndex idx(int row, int column) { return model.index(row, column); }
};

}

// --- Accesseurs de base -----------------------------------------------------

TEST_F(SelectionModelListTest, ExposesModelAndMode)
{
    EXPECT_EQ(selection.sourceModel(), &model);
    EXPECT_EQ(selection.mode(), SelectionMode::List);
    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_FALSE(selection.currentValue().has_value());
    EXPECT_EQ(selection.currentRef(), nullptr);
}

// --- Alternance par index (List) --------------------------------------------

TEST_F(SelectionModelListTest, IndexAlternationTracksValuesAndSignals)
{
    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);

    selection.select(idx(0), QItemSelectionModel::Select);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{10}));
    EXPECT_TRUE(selection.isSelected(10));
    EXPECT_EQ(spy.count(), 1);

    selection.select(idx(0), QItemSelectionModel::Deselect);
    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_FALSE(selection.isSelected(10));
    EXPECT_EQ(spy.count(), 2);

    selection.select(idx(1), QItemSelectionModel::Toggle);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{20}));
    EXPECT_TRUE(selection.isSelected(20));
    EXPECT_EQ(spy.count(), 3);

    selection.select(idx(1), QItemSelectionModel::Toggle);
    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_FALSE(selection.isSelected(20));
    EXPECT_EQ(spy.count(), 4);
}

TEST_F(SelectionModelListTest, SelectionChangedReportsDeltas)
{
    qRegisterMetaType<QItemSelection>("QItemSelection");
    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);

    selection.select(idx(0), QItemSelectionModel::Select);
    ASSERT_EQ(spy.count(), 1);
    {
        const QList<QVariant> args = spy.at(0);
        const QItemSelection selected = args.at(0).value<QItemSelection>();
        const QItemSelection deselected = args.at(1).value<QItemSelection>();
        EXPECT_EQ(selected.indexes().size(), 1);
        EXPECT_TRUE(deselected.indexes().isEmpty());
    }

    selection.select(idx(0), QItemSelectionModel::Deselect);
    ASSERT_EQ(spy.count(), 2);
    {
        const QList<QVariant> args = spy.at(1);
        const QItemSelection selected = args.at(0).value<QItemSelection>();
        const QItemSelection deselected = args.at(1).value<QItemSelection>();
        EXPECT_TRUE(selected.indexes().isEmpty());
        EXPECT_EQ(deselected.indexes().size(), 1);
    }
}

// --- Alternance par valeur (List) -------------------------------------------

TEST_F(SelectionModelListTest, ValueAlternationTracksStateAndSignals)
{
    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);

    selection.select(20, QItemSelectionModel::Select);
    EXPECT_TRUE(selection.isSelected(20));
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{20}));
    EXPECT_EQ(spy.count(), 1);

    selection.deselect(20);
    EXPECT_FALSE(selection.isSelected(20));
    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_EQ(spy.count(), 2);

    selection.select(30, QItemSelectionModel::Toggle);
    EXPECT_TRUE(selection.isSelected(30));
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{30}));
    EXPECT_EQ(spy.count(), 3);

    selection.deselect(30);
    EXPECT_FALSE(selection.isSelected(30));
    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_EQ(spy.count(), 4);
}

// --- Cohérence croisée index <-> valeur (List) ------------------------------

TEST_F(SelectionModelListTest, MixedIndexAndValuePathsStayConsistent)
{
    selection.select(10, QItemSelectionModel::Select); // par valeur
    EXPECT_TRUE(selection.isSelected(10));
    EXPECT_TRUE(selection.isSelected(idx(0))); // surcharge par index héritée

    selection.select(idx(0), QItemSelectionModel::Deselect); // par index
    EXPECT_FALSE(selection.isSelected(10));
    EXPECT_TRUE(selection.selectedValues().empty());

    selection.select(idx(1), QItemSelectionModel::Select); // par index
    EXPECT_TRUE(selection.isSelected(20));
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{20}));

    selection.deselect(20); // par valeur
    EXPECT_FALSE(selection.isSelected(20));
    EXPECT_TRUE(selection.selectedValues().empty());
}

// --- Idempotence et cas limites (List) --------------------------------------

TEST_F(SelectionModelListTest, DeselectUnselectedValueIsNoOp)
{
    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);

    selection.deselect(20);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(selection.selectedValues().empty());
}

TEST_F(SelectionModelListTest, DoubleSelectDoesNotDuplicate)
{
    selection.select(idx(0), QItemSelectionModel::Select);
    selection.select(idx(0), QItemSelectionModel::Select);

    EXPECT_EQ(selection.selectedValues().size(), 1u);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{10}));
}

TEST_F(SelectionModelListTest, ClearRemovesEverything)
{
    selection.select(idx(0), QItemSelectionModel::Select);
    selection.select(idx(2), QItemSelectionModel::Select);
    ASSERT_EQ(sorted(selection.selectedValues()), (std::vector<int>{10, 30}));

    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);
    selection.clear();

    EXPECT_TRUE(selection.selectedValues().empty());
    EXPECT_EQ(spy.count(), 1);
}

// --- Doublons de valeur (List) ----------------------------------------------

TEST_F(SelectionModelListTest, DuplicateValueSelectsAndDeselectsAllRows)
{
    model.reset({10, 20, 30, 20});

    selection.select(20, QItemSelectionModel::Select);
    EXPECT_TRUE(selection.isSelected(20));
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{20, 20}));

    selection.deselect(20);
    EXPECT_FALSE(selection.isSelected(20));
    EXPECT_TRUE(selection.selectedValues().empty());
}

// --- Refs (List) ------------------------------------------------------------

TEST_F(SelectionModelListTest, CurrentRefAndValueReflectCurrentIndex)
{
    selection.setCurrentIndex(idx(1), QItemSelectionModel::Select);

    const std::optional<int> value = selection.currentValue();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 20);

    auto ref = selection.currentRef();
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), 20);
}

TEST_F(SelectionModelListTest, SelectedRefsStayValidAcrossSelectDeselectCycles)
{
    selection.select(idx(1), QItemSelectionModel::Select);

    auto refs = selection.selectedRefs();
    ASSERT_EQ(refs.size(), 1u);
    EXPECT_EQ(refs[0]->getValue(), 20);

    selection.select(idx(1), QItemSelectionModel::Deselect);
    selection.select(idx(1), QItemSelectionModel::Select);

    EXPECT_EQ(refs[0]->getValue(), 20);

    QSignalSpy valueChangedSpy(refs[0].get(), &cute::RefBase::valueChanged);
    model.setData(idx(1), 99, cute::ValueRole);
    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(refs[0]->getValue(), 99);
}

// --- Alternance par index (Table) -------------------------------------------

TEST_F(SelectionModelTableTest, IndexAlternationPerCell)
{
    QSignalSpy spy(&selection, &QItemSelectionModel::selectionChanged);

    selection.select(idx(0, 0), QItemSelectionModel::Select);
    selection.select(idx(1, 1), QItemSelectionModel::Select);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{1, 4}));
    EXPECT_EQ(spy.count(), 2);

    selection.select(idx(0, 0), QItemSelectionModel::Toggle);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{4}));

    selection.select(idx(1, 1), QItemSelectionModel::Deselect);
    EXPECT_TRUE(selection.selectedValues().empty());
}

// --- Alternance par valeur (Table) ------------------------------------------

TEST_F(SelectionModelTableTest, ValueSelectionTargetsMatchingCell)
{
    selection.select(3, QItemSelectionModel::Select);
    EXPECT_TRUE(selection.isSelected(3));
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{3}));
    EXPECT_TRUE(selection.isSelected(idx(1, 0))); // surcharge par index

    selection.deselect(3);
    EXPECT_FALSE(selection.isSelected(3));
    EXPECT_TRUE(selection.selectedValues().empty());
}

TEST_F(SelectionModelTableTest, DuplicateValueSelectsAllCells)
{
    model.getMutable(0, 1) = 3; // (0,1) et (1,0) valent désormais 3

    selection.select(3, QItemSelectionModel::Select);
    EXPECT_EQ(sorted(selection.selectedValues()), (std::vector<int>{3, 3}));

    selection.deselect(3);
    EXPECT_TRUE(selection.selectedValues().empty());
}

// --- Refs / current (Table) -------------------------------------------------

TEST_F(SelectionModelTableTest, CurrentValueAndRefForCell)
{
    selection.setCurrentIndex(idx(1, 0), QItemSelectionModel::NoUpdate);

    const std::optional<int> value = selection.currentValue();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 3);

    auto ref = selection.currentRef();
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), 3);
}

TEST_F(SelectionModelTableTest, NoCurrentGivesNulloptAndNullRef)
{
    EXPECT_FALSE(selection.currentValue().has_value());
    EXPECT_EQ(selection.currentRef(), nullptr);
}
