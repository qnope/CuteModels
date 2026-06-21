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
    Q_PROPERTY(bool valid READ valid NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(int age READ age NOTIFY changed)
    Q_PROPERTY(QString email READ email NOTIFY changed)

public:
    PersonRef(cute::ValueModelAccessor<Person> *model, QPersistentModelIndex index);

    bool valid() const;
    QString name() const;
    int age() const;
    QString email() const;

signals:
    void changed();
};

}
