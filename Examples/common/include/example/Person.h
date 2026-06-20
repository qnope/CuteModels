#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>

#include <utility>

namespace example {

struct Person
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(int age MEMBER m_age)
    Q_PROPERTY(QString email MEMBER m_email)

public:
    Person() = default;

    Person(QString name, int age, QString email)
        : m_name(std::move(name))
        , m_age(age)
        , m_email(std::move(email))
    {}

    QString m_name;
    int m_age = 0;
    QString m_email;
};

inline bool operator==(const Person &lhs, const Person &rhs)
{
    return lhs.m_name == rhs.m_name && lhs.m_age == rhs.m_age
           && lhs.m_email == rhs.m_email;
}

inline bool operator!=(const Person &lhs, const Person &rhs)
{
    return !(lhs == rhs);
}

}

Q_DECLARE_METATYPE(example::Person)
