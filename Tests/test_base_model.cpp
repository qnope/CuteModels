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

#include <optional>

using cute::BaseModel;
using cute::ValueRole;

namespace {

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
        BaseModel<int>::multiData(index, roleDataSpan);
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == ValueRole)
                continue;
            if (roleData.role() == Qt::DisplayRole)
                roleData.setData(QVariant::fromValue(m_value));
            else
                roleData.clearData();
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

struct NoDefault
{
    int payload;
    explicit NoDefault(int p) : payload(p) {}
    NoDefault(const NoDefault &) = default;
};

static_assert(!cute::is_compatible_with_value_role_v<NoDefault>,
              "NoDefault must be incompatible with ValueRole for this fixture "
              "to exercise the disabled-ValueRole path");

class NoValueRoleModel : public BaseModel<NoDefault>
{
public:
    using BaseModel<NoDefault>::BaseModel;

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
        BaseModel<NoDefault>::multiData(index, roleDataSpan);
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == Qt::DisplayRole)
                roleData.setData(QVariant::fromValue(m_value.payload));
            else
                roleData.clearData();
        }
    }

protected:
    const NoDefault &getStorageValue(const QModelIndex &) const override { return m_value; }

    void setStorageValue(const QModelIndex &index, NoDefault value) override
    {
        m_value = value;
        emit dataChanged(index, index);
    }

private:
    NoDefault m_value{0};
};

class NoValueRoleModelTest : public ::testing::Test
{
protected:
    NoValueRoleModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

}

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

TEST_F(NoValueRoleModelTest, RoleNamesOmitsValueRole)
{
    EXPECT_FALSE(model.roleNames().contains(ValueRole));
}

TEST_F(NoValueRoleModelTest, ValueRoleReadContributesNothing)
{
    EXPECT_FALSE(model.index(0, 0).data(ValueRole).isValid());
    EXPECT_EQ(model.index(0, 0).data(Qt::DisplayRole).toInt(), 0);
}

TEST_F(NoValueRoleModelTest, SetDataAcceptsNothing)
{
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_FALSE(model.setData(model.index(0, 0), 1, ValueRole));
    EXPECT_FALSE(model.setData(model.index(0, 0), 1, Qt::EditRole));
    EXPECT_FALSE(model.setData(model.index(0, 0), 1, Qt::DisplayRole));
    EXPECT_EQ(changedSpy.count(), 0);
}
