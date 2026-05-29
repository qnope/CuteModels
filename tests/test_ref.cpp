#include "CuteModel/GetAsRefObject.h"
#include "CuteModel/Ref.h"
#include "CuteModel/ValueRole.h"

#include <gtest/gtest.h>

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <memory>

using CuteModel::getAsRefObject;
using CuteModel::Ref;
using CuteModel::ValueRole;

namespace {

// Minimal Core-only list model exposing its values through ValueRole.
class ValueListModel : public QAbstractListModel
{
public:
    explicit ValueListModel(QStringList values, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_values(std::move(values))
    {
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : m_values.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_values.size())
            return {};
        if (role == ValueRole || role == Qt::DisplayRole)
            return m_values.at(index.row());
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid() || index.row() >= m_values.size())
            return false;
        if (role == ValueRole || role == Qt::EditRole) {
            m_values[index.row()] = value.toString();
            emit dataChanged(index, index, {role});
            return true;
        }
        return false;
    }

private:
    QStringList m_values;
};

std::unique_ptr<ValueListModel> makeModel(const QString &value)
{
    return std::make_unique<ValueListModel>(QStringList{value});
}

} // namespace

TEST(Ref, GetValueReadsValueRole)
{
    auto model = makeModel(QStringLiteral("hello"));

    Ref<QString> ref(QPersistentModelIndex(model->index(0, 0)));

    EXPECT_EQ(ref.getValue(), QStringLiteral("hello"));
}

TEST(Ref, EmitsValueChangedOnMatchingDataChanged)
{
    auto model = makeModel(QStringLiteral("a"));

    Ref<QString> ref(QPersistentModelIndex(model->index(0, 0)));

    int changes = 0;
    QObject::connect(&ref, &CuteModel::RefBase::valueChanged, [&] { ++changes; });

    model->setData(model->index(0, 0), QStringLiteral("b"), ValueRole);

    EXPECT_EQ(changes, 1);
    EXPECT_EQ(ref.getValue(), QStringLiteral("b"));
}

TEST(GetAsRefObject, ReturnsNullForInvalidIndex)
{
    EXPECT_EQ(getAsRefObject<>(QModelIndex()), nullptr);
}

TEST(GetAsRefObject, ConstructsTypedRef)
{
    auto model = makeModel(QStringLiteral("world"));

    Ref<QString> *ref = getAsRefObject<Ref<QString>>(model->index(0, 0));

    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(ref->getValue(), QStringLiteral("world"));

    delete ref;
}
