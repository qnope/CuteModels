#include "CuteModel/BasicListModel.h"
#include "CuteModel/MetaObject.h"

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QAbstractItemModelTester>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QModelRoleData>
#include <QObject>
#include <QString>
#include <QVariant>

#include <array>
#include <memory>
#include <vector>

using cute::BasicListModel;
using cute::PropertyRoleBase;

namespace {

struct Gadget
{
    Q_GADGET
    Q_PROPERTY(int width MEMBER m_width)
    Q_PROPERTY(int height MEMBER m_height)

public:
    int m_width = 0;
    int m_height = 0;
};

class MyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title MEMBER m_title)
    Q_PROPERTY(int count MEMBER m_count)

public:
    QString m_title;
    int m_count = 0;
};

int roleFor(const QAbstractItemModel &model, const char *name)
{
    const QHash<int, QByteArray> names = model.roleNames();
    for (auto it = names.constBegin(); it != names.constEnd(); ++it)
        if (it.value() == name)
            return it.key();
    return -1;
}

template <typename T>
class PropertyListModel : public BasicListModel<T>
{
public:
    using BasicListModel<T>::BasicListModel;
    using BasicListModel<T>::data;

    QVariant data(const T &, const QModelIndex &, int) const override { return {}; }
};

}

TEST(PropertyRolesGadgetValue, RoleNamesExposeProperties)
{
    PropertyListModel<Gadget> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    EXPECT_GE(roleFor(model, "width"), PropertyRoleBase);
    EXPECT_GE(roleFor(model, "height"), PropertyRoleBase);
}

TEST(PropertyRolesGadgetValue, DataReadsProperties)
{
    PropertyListModel<Gadget> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    Gadget gadget;
    gadget.m_width = 3;
    gadget.m_height = 4;
    model.push_back(gadget);

    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, roleFor(model, "width")).toInt(), 3);
    EXPECT_EQ(model.data(index, roleFor(model, "height")).toInt(), 4);
}

TEST(PropertyRolesGadgetValue, MultiDataFillsEveryPropertyRole)
{
    PropertyListModel<Gadget> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    Gadget gadget;
    gadget.m_width = 7;
    gadget.m_height = 9;
    model.push_back(gadget);

    std::array<QModelRoleData, 2> roles{
        QModelRoleData(roleFor(model, "width")),
        QModelRoleData(roleFor(model, "height"))};
    model.multiData(model.index(0, 0), roles);

    EXPECT_EQ(roles[0].data().toInt(), 7);
    EXPECT_EQ(roles[1].data().toInt(), 9);
}

TEST(PropertyRolesGadgetPointer, DataReadsPropertiesThroughPointer)
{
    std::vector<std::unique_ptr<Gadget>> storage;
    PropertyListModel<Gadget *> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    auto gadget = std::make_unique<Gadget>();
    gadget->m_width = 5;
    gadget->m_height = 6;
    model.push_back(gadget.get());
    storage.push_back(std::move(gadget));

    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, roleFor(model, "width")).toInt(), 5);
    EXPECT_EQ(model.data(index, roleFor(model, "height")).toInt(), 6);
}

TEST(PropertyRolesGadgetPointer, NullPointerYieldsInvalidVariant)
{
    PropertyListModel<Gadget *> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    model.push_back(nullptr);

    EXPECT_FALSE(model.data(model.index(0, 0), roleFor(model, "width")).isValid());
}

TEST(PropertyRolesObjectPointer, RoleNamesIncludeInheritedObjectName)
{
    PropertyListModel<MyObject *> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    EXPECT_GE(roleFor(model, "title"), PropertyRoleBase);
    EXPECT_GE(roleFor(model, "count"), PropertyRoleBase);
    EXPECT_GE(roleFor(model, "objectName"), PropertyRoleBase);
}

TEST(PropertyRolesObjectPointer, DataReadsPropertiesThroughPointer)
{
    std::vector<std::unique_ptr<MyObject>> storage;
    PropertyListModel<MyObject *> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    auto object = std::make_unique<MyObject>();
    object->m_title = QStringLiteral("hello");
    object->m_count = 7;
    model.push_back(object.get());
    storage.push_back(std::move(object));

    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, roleFor(model, "title")).toString(),
              QStringLiteral("hello"));
    EXPECT_EQ(model.data(index, roleFor(model, "count")).toInt(), 7);
}

TEST(PropertyRolesObjectPointer, NullPointerYieldsInvalidVariant)
{
    PropertyListModel<MyObject *> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    model.push_back(nullptr);

    EXPECT_FALSE(model.data(model.index(0, 0), roleFor(model, "title")).isValid());
}

TEST(PropertyRolesObjectUniquePtr, DataReadsPropertiesThroughUniquePtr)
{
    PropertyListModel<std::unique_ptr<MyObject>> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    auto object = std::make_unique<MyObject>();
    object->m_title = QStringLiteral("owned");
    object->m_count = 11;
    model.push_back(std::move(object));

    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, roleFor(model, "title")).toString(),
              QStringLiteral("owned"));
    EXPECT_EQ(model.data(index, roleFor(model, "count")).toInt(), 11);
}

TEST(PropertyRolesObjectUniquePtr, NullUniquePtrYieldsInvalidVariant)
{
    PropertyListModel<std::unique_ptr<MyObject>> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    model.push_back(nullptr);

    EXPECT_FALSE(model.data(model.index(0, 0), roleFor(model, "title")).isValid());
}

TEST(PropertyRolesObjectSharedPtr, DataReadsPropertiesThroughSharedPtr)
{
    PropertyListModel<std::shared_ptr<MyObject>> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    auto object = std::make_shared<MyObject>();
    object->m_title = QStringLiteral("shared");
    object->m_count = 13;
    model.push_back(object);

    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, roleFor(model, "title")).toString(),
              QStringLiteral("shared"));
    EXPECT_EQ(model.data(index, roleFor(model, "count")).toInt(), 13);
}

TEST(PropertyRolesObjectSharedPtr, MultiDataFillsEveryPropertyRole)
{
    PropertyListModel<std::shared_ptr<MyObject>> model;
    QAbstractItemModelTester tester{
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    auto object = std::make_shared<MyObject>();
    object->m_title = QStringLiteral("multi");
    object->m_count = 21;
    model.push_back(object);

    std::array<QModelRoleData, 2> roles{
        QModelRoleData(roleFor(model, "title")),
        QModelRoleData(roleFor(model, "count"))};
    model.multiData(model.index(0, 0), roles);

    EXPECT_EQ(roles[0].data().toString(), QStringLiteral("multi"));
    EXPECT_EQ(roles[1].data().toInt(), 21);
}

#include "test_property_roles.moc"
