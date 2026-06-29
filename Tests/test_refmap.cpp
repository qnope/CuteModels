#include "CuteModel/BasicListModel.h"
#include "CuteModel/RefMap.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QMetaType>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <memory>
#include <utility>

using cute::BasicListModel;
using cute::RefMap;

namespace {

struct Person
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(int age MEMBER m_age)

public:
    Person() = default;

    Person(QString name, int age)
        : m_name(std::move(name))
        , m_age(age)
    {}

    QString m_name;
    int m_age = 0;
};

inline bool operator==(const Person &lhs, const Person &rhs)
{
    return lhs.m_name == rhs.m_name && lhs.m_age == rhs.m_age;
}

class PersonListModel : public BasicListModel<Person>
{
public:
    using BasicListModel<Person>::BasicListModel;
    using BasicListModel<Person>::data;

    QVariant data(const Person &, const QModelIndex &, int) const override { return {}; }
};

}

Q_DECLARE_METATYPE(Person)

namespace {

class RefMapTest : public ::testing::Test
{
protected:
    PersonListModel model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    void SetUp() override { model.push_back(Person(QStringLiteral("Alice"), 30)); }

    QModelIndex index() const { return model.index(0, 0); }

    const cute::ValueModelAccessor<Person> &accessor() const { return model; }
};

}

TEST_F(RefMapTest, ExposesMetaPropertiesAsKeys)
{
    auto ref = model.getRef<>(index());
    ASSERT_NE(ref, nullptr);

    RefMap<Person> map(std::move(ref));

    EXPECT_EQ(map.value(QStringLiteral("name")).toString(), QStringLiteral("Alice"));
    EXPECT_EQ(map.value(QStringLiteral("age")).toInt(), 30);

    const Person value = map.value(QStringLiteral("value")).value<Person>();
    EXPECT_EQ(value, Person(QStringLiteral("Alice"), 30));
}

TEST_F(RefMapTest, RefreshesWhenStorageChanges)
{
    auto driver = model.getRef<>(index());
    ASSERT_NE(driver, nullptr);

    RefMap<Person> map(model.getRef<>(index()));

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    driver->setValue(Person(QStringLiteral("Bob"), 41));

    EXPECT_EQ(dataChangedSpy.count(), 1);
    EXPECT_EQ(map.value(QStringLiteral("name")).toString(), QStringLiteral("Bob"));
    EXPECT_EQ(map.value(QStringLiteral("age")).toInt(), 41);
}

TEST_F(RefMapTest, WritesPropertyBackToModel)
{
    RefMap<Person> map(model.getRef<>(index()));

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    map.setProperty("name", QStringLiteral("Bob"));

    EXPECT_EQ(dataChangedSpy.count(), 1);

    const Person &stored = accessor().getStorageValue(index());
    EXPECT_EQ(stored.m_name, QStringLiteral("Bob"));
    EXPECT_EQ(stored.m_age, 30);
}

TEST_F(RefMapTest, WritesValueKeyBackToModel)
{
    RefMap<Person> map(model.getRef<>(index()));

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    map.setProperty("value", QVariant::fromValue(Person(QStringLiteral("Carol"), 52)));

    EXPECT_EQ(dataChangedSpy.count(), 1);

    EXPECT_EQ(accessor().getStorageValue(index()), Person(QStringLiteral("Carol"), 52));
}

#include "test_refmap.moc"
