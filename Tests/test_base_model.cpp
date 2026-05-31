#include "CuteModel/BaseModel.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QList>
#include <QMimeData>
#include <QModelIndex>
#include <QModelRoleData>
#include <QSignalSpy>
#include <QVariant>

#include <optional>

using cute::BaseModel;
using cute::ValueRole;

namespace {

// Minimal BaseModel<int> with a single storage cell. Exists only to lock the
// BaseModel contract (setData role policy + setStorageValue write hook,
// roleNames, value-based drop adapters, and the ValueRole read path that
// mimeData / drops rely on) independently from BasicListModel's list-shaped
// structure. Implements its own multiData (exposing ValueRole) since role
// projection is not part of BaseModel's responsibilities.
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

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QModelRoleData roleData(role);
        multiData(index, roleData);
        return roleData.data();
    }

    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid))
            return;
        const int &value = m_value;
        for (QModelRoleData &roleData : roleDataSpan) {
            if (roleData.role() == ValueRole || roleData.role() == Qt::DisplayRole)
                roleData.setData(QVariant::fromValue(value));
            else
                roleData.clearData();
        }
    }

    int storage() const { return m_value; }

protected:
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

TEST_F(BaseModelTest, DefaultCanDropAndDropRejectAll)
{
    QMimeData payload;
    payload.setText(QStringLiteral("ignored"));
    EXPECT_FALSE(model.canDropMimeData(&payload, Qt::CopyAction, -1, 0, QModelIndex()));
    EXPECT_FALSE(model.dropMimeData(&payload, Qt::CopyAction, -1, 0, QModelIndex()));
}
