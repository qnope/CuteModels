#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>

namespace examples {

struct Person
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int age MEMBER age)
    Q_PROPERTY(QString city MEMBER city)
public:
    QString name;
    int age = 0;
    QString city;
};

inline QString personSummary(const Person &person)
{
    return QStringLiteral("%1, %2 (%3)")
        .arg(person.name)
        .arg(person.age)
        .arg(person.city);
}

}

Q_DECLARE_METATYPE(examples::Person)
