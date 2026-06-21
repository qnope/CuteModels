#pragma once

#include "example/Person.h"

#include "CuteModel/BasicListModel.h"

#include <QModelIndex>
#include <QVariant>

#include <vector>

namespace example {

class PersonListModel : public cute::BasicListModel<Person>
{
public:
    using cute::BasicListModel<Person>::BasicListModel;
    using cute::BasicListModel<Person>::data;

    QVariant data(const Person &, const QModelIndex &, int) const override { return {}; }
};

std::vector<Person> samplePeople();

}
