#pragma once

#include "CuteModel/Ref.h"

#include <QModelIndex>
#include <QPersistentModelIndex>

namespace CuteModel {

// Builds a RefBase-derived object that follows `index`. The returned object is
// parented to the index's model, so it lives as long as the model unless the
// caller reparents or deletes it. Default to Ref<QVariant> when no concrete type
// is requested.
template <typename Default = Ref<QVariant>>
Default *getAsRefObject(const QModelIndex &index)
{
    static_assert(std::is_base_of_v<RefBase, Default>,
                  "getAsRefObject<Default> requires Default to derive from RefBase");

    if (!index.isValid())
        return nullptr;

    return new Default(QPersistentModelIndex(index));
}

} // namespace CuteModel
