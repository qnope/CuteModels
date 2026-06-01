#include "CuteModel/BaseModel.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QModelIndex>
#include <QModelRoleData>
#include <QSignalSpy>
#include <QVariant>

#include <memory>
#include <optional>

using cute::BaseModel;
using cute::ValueRole;

namespace {

// Minimal BaseModel<int> with a single storage cell. Exists only to lock the
// BaseModel contract (setData role policy + setStorageValue write hook,
// roleNames, the value-based drag source, the data()/multiData() read path and
// its ValueRole handling) independently from BasicListModel's list-shaped
// structure. ValueRole and data(QModelIndex) come from BaseModel; this only
// adds DisplayRole projection on top of the base multiData.
class SingleCellModel : public BaseModel<int>
{
public:
    using BaseModel<int>::BaseModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || row != 0 || column != 0)
            return {};
        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        // ValueRole is filled by the base; this fixture only adds DisplayRole.
        BaseModel<int>::multiData(index, roleDataSpan);
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == Qt::DisplayRole)
                roleData.setData(QVariant::fromValue(m_value));
        }
    }

    int storage() const { return m_value; }

protected:
    const int &getStorageValue(const QModelIndex &) const override { return m_value; }

    void setStorageValue(const QModelIndex &index, int value) override
    {
        m_value = value;
        emit dataChanged(index, index);
    }

private:
    int m_value = 0;
};

class BaseModelTest : public ::testing::Test
{
protected:
    SingleCellModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

// Single-cell model over a move-only T (std::unique_ptr<int>), which is NOT
// is_compatible_with_value_role. It exercises the ValueRole opt-out: the base
// multiData / data / setData / roleNames must all compile (the ValueRole bodies
// must be discarded by if constexpr, never instantiating QVariant::fromValue /
// value<T>) and must behave as if ValueRole did not exist.
class MoveOnlyCellModel : public BaseModel<std::unique_ptr<int>>
{
public:
    using BaseModel<std::unique_ptr<int>>::BaseModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || row != 0 || column != 0)
            return {};
        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

protected:
    const std::unique_ptr<int> &getStorageValue(const QModelIndex &) const override
    {
        return m_value;
    }

    void setStorageValue(const QModelIndex &, std::unique_ptr<int> value) override
    {
        m_value = std::move(value);
    }

private:
    std::unique_ptr<int> m_value;
};

class MoveOnlyCellModelTest : public ::testing::Test
{
protected:
    MoveOnlyCellModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

} // namespace

TEST_F(BaseModelTest, ValueRoleReadsStoredValue)
{
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 0);
}

TEST_F(BaseModelTest, ValueRoleWritesToStorage)
{
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    ASSERT_TRUE(model.setData(model.index(0, 0), 42, ValueRole));

    EXPECT_EQ(model.storage(), 42);
    EXPECT_EQ(model.index(0, 0).data(ValueRole).toInt(), 42);
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toInt(), 42);
    ASSERT_EQ(changedSpy.count(), 1);
    const QList<QVariant> args = changedSpy.at(0);
    EXPECT_EQ(args.at(0).value<QModelIndex>(), model.index(0, 0));
    EXPECT_TRUE(args.at(2).value<QList<int>>().isEmpty());
}

TEST_F(BaseModelTest, SetDataRejectsInvalidVariant)
{
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    // A default-constructed QVariant is invalid and has no convertibility,
    // so setData refuses it.
    EXPECT_FALSE(model.setData(model.index(0, 0), QVariant(), ValueRole));
    EXPECT_EQ(model.storage(), 0);
    EXPECT_EQ(changedSpy.count(), 0);
}

TEST_F(BaseModelTest, SetDataWithUnsupportedRoleTerminates)
{
    EXPECT_DEATH({ (void)model.setData(model.index(0, 0), 1, Qt::DisplayRole); }, "");
    EXPECT_DEATH({ (void)model.setData(model.index(0, 0), 1, Qt::EditRole); }, "");
    EXPECT_DEATH({ (void)model.setData(model.index(0, 0), 1, Qt::UserRole + 5); }, "");
}

TEST_F(BaseModelTest, RoleNamesAdvertisesValueRole)
{
    const auto names = model.roleNames();
    ASSERT_TRUE(names.contains(ValueRole));
    EXPECT_EQ(names.value(ValueRole), QByteArrayLiteral("value"));
}

TEST_F(BaseModelTest, DefaultMimeDataIsNull)
{
    EXPECT_EQ(model.mimeData({model.index(0, 0)}), nullptr);
}

// ---------- ValueRole opt-out for incompatible T ----------

TEST_F(MoveOnlyCellModelTest, RoleNamesOmitsValueRole)
{
    // T is move-only, so ValueRole cannot travel through a QVariant — roleNames
    // must not advertise it.
    EXPECT_FALSE(model.roleNames().contains(ValueRole));
}

TEST_F(MoveOnlyCellModelTest, ValueRoleReadsNothing)
{
    // The base multiData never fills ValueRole for an incompatible T.
    EXPECT_FALSE(model.index(0, 0).data(ValueRole).isValid());
}

TEST_F(MoveOnlyCellModelTest, SetDataIsRejectedNotTerminated)
{
    // No writable role exists, so setData falls through to the base and simply
    // returns false (no std::terminate) regardless of the role.
    EXPECT_FALSE(model.setData(model.index(0, 0), QVariant(), ValueRole));
    EXPECT_FALSE(model.setData(model.index(0, 0), QVariant(), Qt::DisplayRole));
}
