#pragma once

#include <array>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace CuteModel {

// Stores a value of an arbitrary type T inside a trivially-copyable byte buffer so
// it can travel through a QVariant even when T is not registered with
// Q_DECLARE_METATYPE. The wrapper itself owns the contained T's lifetime and is
// the type that should be declared as a metatype.
template <typename T>
class VariantWrapper
{
public:
    VariantWrapper() { ::new (storage()) T(); }

    explicit VariantWrapper(const T &value) { ::new (storage()) T(value); }
    explicit VariantWrapper(T &&value) { ::new (storage()) T(std::move(value)); }

    VariantWrapper(const VariantWrapper &other) { ::new (storage()) T(other.get()); }

    VariantWrapper(VariantWrapper &&other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        ::new (storage()) T(std::move(other.get()));
    }

    VariantWrapper &operator=(const VariantWrapper &other)
    {
        if (this != &other)
            get() = other.get();
        return *this;
    }

    VariantWrapper &operator=(VariantWrapper &&other)
        noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (this != &other)
            get() = std::move(other.get());
        return *this;
    }

    ~VariantWrapper() { get().~T(); }

    T &get() noexcept { return *std::launder(reinterpret_cast<T *>(storage())); }
    const T &get() const noexcept
    {
        return *std::launder(reinterpret_cast<const T *>(storage()));
    }

    T &operator*() noexcept { return get(); }
    const T &operator*() const noexcept { return get(); }

    T *operator->() noexcept { return &get(); }
    const T *operator->() const noexcept { return &get(); }

private:
    std::byte *storage() noexcept { return m_storage.data(); }
    const std::byte *storage() const noexcept { return m_storage.data(); }

    alignas(T) std::array<std::byte, sizeof(T)> m_storage;
};

} // namespace CuteModel
