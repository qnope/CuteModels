#pragma once

#include "example/Person.h"

#include "CuteModel/ValueModelAccessor.h"

#include <QObject>
#include <QPersistentModelIndex>
#include <QString>

namespace example {

class PersonRef : public cute::ValueModelAccessor<Person>::Ref
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY valueChanged)
    Q_PROPERTY(QString name READ name NOTIFY valueChanged)
    Q_PROPERTY(int age READ age NOTIFY valueChanged)
    Q_PROPERTY(QString email READ email NOTIFY valueChanged)

public:
    PersonRef(cute::ValueModelAccessor<Person> *model, QPersistentModelIndex index);

    bool valid() const;
    QString name() const;
    int age() const;
    QString email() const;
};

}
