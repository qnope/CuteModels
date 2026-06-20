#include "example/PersonRef.h"

namespace example {

PersonRef::PersonRef(std::unique_ptr<ValueRef> ref, QObject *parent)
    : QObject(parent)
    , m_ref(std::move(ref))
{
    if (m_ref) {
        connect(m_ref.get(), &cute::RefBase::valueChanged, this, &PersonRef::changed);
        connect(m_ref.get(), &cute::RefBase::underlyingValueDestroyed,
                this, &PersonRef::changed);
    }
}

bool PersonRef::valid() const
{
    return m_ref && m_ref->index().isValid();
}

QString PersonRef::name() const
{
    return valid() ? m_ref->getValue().m_name : QString();
}

int PersonRef::age() const
{
    return valid() ? m_ref->getValue().m_age : 0;
}

QString PersonRef::email() const
{
    return valid() ? m_ref->getValue().m_email : QString();
}

}
