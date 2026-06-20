#pragma once

#include "example/Person.h"

#include "CuteModel/RefBase.h"
#include "CuteModel/ValueModelAccessor.h"

#include <QObject>
#include <QString>

#include <memory>

namespace example {

class PersonRef : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(int age READ age NOTIFY changed)
    Q_PROPERTY(QString email READ email NOTIFY changed)

public:
    using ValueRef = cute::ValueModelAccessor<Person>::Ref;

    explicit PersonRef(std::unique_ptr<ValueRef> ref, QObject *parent = nullptr);

    bool valid() const;
    QString name() const;
    int age() const;
    QString email() const;

signals:
    void changed();

private:
    std::unique_ptr<ValueRef> m_ref;
};

}
