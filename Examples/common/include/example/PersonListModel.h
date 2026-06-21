#pragma once

#include "example/Person.h"

#include "CuteModel/BasicListModel.h"

#include <vector>

namespace example {

class PersonListModel : public cute::BasicListModel<Person>
{
public:
    using cute::BasicListModel<Person>::BasicListModel;
};

std::vector<Person> samplePeople();

}
