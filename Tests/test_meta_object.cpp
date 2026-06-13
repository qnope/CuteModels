#include "CuteModel/MetaObject.h"

#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QString>

#include <algorithm>
#include <string>

using cute::is_meta_object_compatible;
using cute::is_meta_object_compatible_v;
using cute::metaProperties;

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

struct PlainStruct
{
    int value = 0;
};

static_assert(is_meta_object_compatible_v<Gadget>);
static_assert(is_meta_object_compatible_v<MyObject>);
static_assert(is_meta_object_compatible_v<QObject>);

static_assert(!is_meta_object_compatible_v<PlainStruct>);
static_assert(!is_meta_object_compatible_v<int>);
static_assert(!is_meta_object_compatible_v<std::string>);

static_assert(is_meta_object_compatible<Gadget>::value ==
              is_meta_object_compatible_v<Gadget>);

bool containsProperty(const std::vector<QMetaProperty> &properties,
                      const char *name)
{
    return std::any_of(properties.begin(), properties.end(),
                       [name](const QMetaProperty &property) {
                           return std::string(property.name()) == name;
                       });
}

}

TEST(MetaObject, GadgetExposesOwnProperties)
{
    const auto properties = metaProperties<Gadget>();

    EXPECT_TRUE(containsProperty(properties, "width"));
    EXPECT_TRUE(containsProperty(properties, "height"));
}

TEST(MetaObject, ObjectExposesDeclaredProperties)
{
    const auto properties = metaProperties<MyObject>();

    EXPECT_TRUE(containsProperty(properties, "title"));
    EXPECT_TRUE(containsProperty(properties, "count"));
}

TEST(MetaObject, ObjectIncludesInheritedProperties)
{
    const auto properties = metaProperties<MyObject>();

    EXPECT_TRUE(containsProperty(properties, "objectName"));
}

#include "test_meta_object.moc"
