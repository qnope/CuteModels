#include "CuteModel/BasicListModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <memory>

using cute::BasicListModel;
using cute::ValueRole;

namespace {

class ValueListModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, int column, int role) const override
    {
        if (column == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
            return value;
        return {};
    }
};

std::unique_ptr<ValueListModel> makeModel(const QString &value)
{
    auto model = std::make_unique<ValueListModel>();
    model->push_back(value);
    return model;
}

}

TEST(Ref, GetValueReadsValueRole)
{
    auto model = makeModel(QStringLiteral("hello"));

    auto ref = model->getRef<>(model->index(0, 0));

    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), QStringLiteral("hello"));
}

TEST(Ref, EmitsValueChangedOnMatchingDataChanged)
{
    auto model = makeModel(QStringLiteral("a"));

    auto ref = model->getRef<>(model->index(0, 0));
    ASSERT_NE(ref, nullptr);

    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);

    model->setData(model->index(0, 0), QStringLiteral("b"), ValueRole);

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), QStringLiteral("b"));
}

TEST(Ref, SetValueWritesThroughValueRole)
{
    auto model = makeModel(QStringLiteral("a"));

    auto ref = model->getRef<>(model->index(0, 0));
    ASSERT_NE(ref, nullptr);

    ref->setValue(QStringLiteral("b"));
    EXPECT_EQ(ref->getValue(), QStringLiteral("b"));
    EXPECT_EQ(model->index(0, 0).data(ValueRole).toString(), QStringLiteral("b"));
}

TEST(Ref, SetValueEmitsValueChanged)
{
    auto model = makeModel(QStringLiteral("a"));

    auto ref = model->getRef<>(model->index(0, 0));
    ASSERT_NE(ref, nullptr);

    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);

    ref->setValue(QStringLiteral("b"));

    EXPECT_EQ(valueChangedSpy.count(), 1);
}

TEST(GetRef, ReturnsNullForInvalidIndex)
{
    auto model = makeModel(QStringLiteral("x"));

    EXPECT_EQ(model->getRef<>(QModelIndex()), nullptr);
}

TEST(GetRef, ConstructsTypedRef)
{
    auto model = makeModel(QStringLiteral("world"));

    std::unique_ptr<ValueListModel::Ref> ref =
        model->getRef<ValueListModel::Ref>(model->index(0, 0));

    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), QStringLiteral("world"));
    EXPECT_EQ(ref->parent(), nullptr);
}
