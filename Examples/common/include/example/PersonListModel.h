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

    QVariant data(const Person &value, const QModelIndex &index, int role) const override;
};

std::vector<Person> samplePeople();

}
