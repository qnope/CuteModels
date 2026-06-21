#include "example/PersonRef.h"

#include <utility>

namespace example {

PersonRef::PersonRef(cute::ValueModelAccessor<Person> *model, QPersistentModelIndex index)
    : Ref(model, std::move(index))
{
    connect(this, &cute::RefBase::valueChanged, this, &PersonRef::changed);
    connect(this, &cute::RefBase::underlyingValueDestroyed, this, &PersonRef::changed);
}

bool PersonRef::valid() const
{
    return index().isValid();
}

QString PersonRef::name() const
{
    return valid() ? getValue().m_name : QString();
}

int PersonRef::age() const
{
    return valid() ? getValue().m_age : 0;
}

QString PersonRef::email() const
{
    return valid() ? getValue().m_email : QString();
}

}
