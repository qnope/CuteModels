#include "CuteModel/BasicListModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/ValueModelAccessor.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <memory>
#include <type_traits>

using cute::BasicListModel;
using cute::ValueModelAccessor;

namespace {

class StringListModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, const QModelIndex &index, int role) const override
    {
        if (index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
            return value;
        return {};
    }
};

std::unique_ptr<StringListModel> makeModel(const QString &value)
{
    auto model = std::make_unique<StringListModel>();
    model->push_back(value);
    return model;
}

}

// The segregation contract: ValueModelAccessor<T> is a standalone, polymorphic,
// non-QObject interface so a future Proxy<T> can co-inherit it alongside an item model.
static_assert(!std::is_base_of_v<QObject, ValueModelAccessor<int>>,
              "ValueModelAccessor must not be a QObject");
static_assert(std::is_polymorphic_v<ValueModelAccessor<int>>,
              "ValueModelAccessor must be polymorphic");
static_assert(std::is_base_of_v<ValueModelAccessor<QString>, StringListModel>,
              "AbstractSourceModel subclasses must derive from ValueModelAccessor<T>");

TEST(ValueModelAccessor, ReadsThroughInterfaceReference)
{
    auto model = makeModel(QStringLiteral("hello"));
    QAbstractItemModelTester tester(model.get());

    const ValueModelAccessor<QString> &accessor = *model;

    EXPECT_EQ(accessor.getStorageValue(model->index(0, 0)), QStringLiteral("hello"));
}

TEST(ValueModelAccessor, GetRefThroughInterfaceReference)
{
    auto model = makeModel(QStringLiteral("a"));
    QAbstractItemModelTester tester(model.get());

    ValueModelAccessor<QString> &accessor = *model;

    auto ref = accessor.getRef<>(model->index(0, 0));
    ASSERT_NE(ref, nullptr);

    QSignalSpy valueChangedSpy(ref.get(), &cute::RefBase::valueChanged);

    ref->setValue(QStringLiteral("b"));

    EXPECT_EQ(valueChangedSpy.count(), 1);
    EXPECT_EQ(ref->getValue(), QStringLiteral("b"));
    EXPECT_EQ(accessor.getStorageValue(model->index(0, 0)), QStringLiteral("b"));
}

TEST(ValueModelAccessor, CrossCastsToItemModel)
{
    // Every concrete model co-inherits QAbstractItemModel; the cross-cast a future
    // Proxy<T> relies on must succeed from a ValueModelAccessor<T> reference.
    auto model = makeModel(QStringLiteral("x"));

    ValueModelAccessor<QString> &accessor = *model;

    EXPECT_NE(dynamic_cast<QAbstractItemModel *>(&accessor), nullptr);
}
