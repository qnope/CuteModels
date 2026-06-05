#pragma once

#include <stdexcept>

namespace cute {

class NullObjectException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

}
