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

    QVariant data(const int &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return QVariant::fromValue(value);
        return {};
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

    QVariant data(const NoDefault &value, const QModelIndex &, int role) const override
    {
        if (role == Qt::DisplayRole)
            return QVariant::fromValue(value.payload);
        return {};
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

class TwoColumnModel : public BaseModel<int>
{
public:
    using BaseModel<int>::BaseModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 2;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || row != 0 || column < 0 || column > 1)
            return {};
        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override { return {}; }

    QVariant data(const int &, const QModelIndex &, int) const override { return {}; }

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

class TwoColumnModelTest : public ::testing::Test
{
protected:
    TwoColumnModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

// Overrides the per-element flags customization point to prove BaseModel hands
// it the stored value and the full index (not just the column).
class CustomFlagsModel : public TwoColumnModel
{
public:
    using TwoColumnModel::TwoColumnModel;
    using TwoColumnModel::flags;

    Qt::ItemFlags flags(const int &value, const QModelIndex &index) const override
    {
        Qt::ItemFlags base = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (value > 0 && index.column() == 1)
            base |= Qt::ItemIsUserCheckable;
        return base;
    }
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

TEST_F(BaseModelTest, SetDataWithUnsupportedRoleReturnsFalse)
{
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    EXPECT_FALSE(model.setData(model.index(0, 0), 1, Qt::DisplayRole));
    EXPECT_FALSE(model.setData(model.index(0, 0), 1, Qt::EditRole));
    EXPECT_FALSE(model.setData(model.index(0, 0), 1, Qt::UserRole + 5));

    EXPECT_EQ(changedSpy.count(), 0);
    EXPECT_EQ(model.storage(), 0);
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

TEST_F(TwoColumnModelTest, FlagsDefaultToSelectableAndEnabled)
{
    const Qt::ItemFlags expected = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    EXPECT_EQ(model.flags(model.index(0, 0)), expected);
    EXPECT_EQ(model.flags(model.index(0, 1)), expected);
}

TEST_F(TwoColumnModelTest, FlagsOnInvalidIndexAreNoneByDefault)
{
    EXPECT_EQ(model.flags(QModelIndex()), Qt::NoItemFlags);
    EXPECT_EQ(model.rootFlags(), Qt::NoItemFlags);
}

TEST_F(TwoColumnModelTest, RootFlagsAreSettableAndReturnedForInvalidIndex)
{
    model.setRootFlags(Qt::ItemIsDropEnabled);

    EXPECT_EQ(model.rootFlags(), Qt::ItemIsDropEnabled);
    EXPECT_EQ(model.flags(QModelIndex()), Qt::ItemIsDropEnabled);
    // Valid indices still resolve through the per-element customization point.
    EXPECT_TRUE(model.flags(model.index(0, 0)) & Qt::ItemIsSelectable);
}

TEST(BaseModelFlags, OverridenFlagsReceiveValueAndIndex)
{
    CustomFlagsModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    EXPECT_FALSE(model.flags(model.index(0, 1)) & Qt::ItemIsUserCheckable);

    ASSERT_TRUE(model.setData(model.index(0, 0), 5, ValueRole));

    EXPECT_TRUE(model.flags(model.index(0, 1)) & Qt::ItemIsUserCheckable);
    EXPECT_FALSE(model.flags(model.index(0, 0)) & Qt::ItemIsUserCheckable);
}
