#pragma once

#include <ZSTL/type_traits.hpp> // zstl::contain_type, zstl::type_index_v

#include <cstddef> // std::size_t
#include <cstdint> // std::uintptr_t, std::uint64_t
#include <utility> // std::forward
#include <type_traits> // std::integral_constant


namespace zstl {

namespace detail::tagged_ptr {

template <typename Func, typename T>
auto dispatch_call(
    Func &&func,
    void *ptr,
    [[maybe_unused]] std::size_t index
) {
    return func(static_cast<T*>(ptr));
}

template <typename Func, typename T>
auto dispatch_call(
    Func &&func,
    const void *ptr,
    [[maybe_unused]] std::size_t index
) {
    return func(static_cast<const T*>(ptr));
}

template <
    typename Func,
    typename T0,
    typename... Ts
>
    requires (sizeof...(Ts) > 0uz)
auto dispatch_call(
    Func &&func,
    void *ptr,
    std::size_t index
) {
    switch (index) {
        case 0uz: {
            return func(static_cast<T0*>(ptr));
        }
        default: {
            return dispatch_call<Func, Ts...>(
                std::forward<Func>(func),
                ptr,
                index - 1uz
            );
        }
    }
}

template <
    typename Func,
    typename T0,
    typename... Ts
>
    requires (sizeof...(Ts) > 0uz)
auto dispatch_call(
    Func &&func,
    const void *ptr,
    std::size_t index
) {
    switch (index) {
        case 0uz: {
            return func(static_cast<const T0*>(ptr));
        }
        default: {
            return dispatch_call<Func, Ts...>(
                std::forward<Func>(func),
                ptr,
                index - 1uz
            );
        }
    }
}

}; // namespace detail::tagged_ptr end


// tagged_ptr
template <typename... Ts>
class tagged_ptr {
private:
    static_assert(
        sizeof(std::uintptr_t) >= sizeof(std::uint64_t),
        "tagged_ptr expect `std::uintptr_t` to have at least 64 bits"
    );

    static constexpr std::uint64_t TAG_SHIFT { 59uz };
    static constexpr std::uintptr_t GET_PTR_MASK = (1uz << TAG_SHIFT) - 1uz;
    std::uintptr_t tagged_address { 0uz };

public:
    tagged_ptr()
        : tagged_ptr(nullptr)
    {}

    tagged_ptr(std::nullptr_t)
        : tagged_address { 0uz }
    {}

    template <typename T>
        requires contain_type<T, Ts...>
    tagged_ptr(const T *ptr)
        : tagged_address {
            reinterpret_cast<std::uintptr_t>(static_cast<const void*>(ptr))
            | (static_cast<std::uintptr_t>(get_type_tag<T>()) << TAG_SHIFT)
        }
    {}

    bool operator== (const tagged_ptr &other) const {
        return (tagged_address == other.tagged_address)
            && (tag() == other.tag());
    }

    bool operator!= (const tagged_ptr &other) const {
        return (tagged_address != other.tagged_address)
            || (tag() != other.tag());
    }

    template <typename T>
        requires contain_type<T, std::nullptr_t, Ts...>
    static constexpr std::size_t get_type_tag() {
        return type_index_v<T, std::nullptr_t, Ts...>;
    }

    template <typename T>
        requires contain_type<T, Ts...>
    bool points_to_type() const {
        return tag() == get_type_tag<T>();
    }

    template <typename T>
        requires contain_type<T, Ts...>
    const T *cast() const {
        return points_to_type<T>()
            ? static_cast<const T*>(ptr())
            : nullptr;
    }

    template <typename T>
        requires contain_type<T, Ts...>
    T *cast() {
        return points_to_type<T>()
            ? static_cast<T*>(ptr())
            : nullptr;
    }

    template <typename T>
        requires contain_type<T, Ts...>
    const T *cast_unchecked() const {
        return static_cast<const T*>(ptr());
    }

    template <typename T>
        requires contain_type<T, Ts...>
    T *cast_unchecked() {
        return static_cast<T*>(ptr());
    }

    template <typename Func>
    decltype(auto) call(Func &&func) {
        return detail::tagged_ptr::dispatch_call<Func, Ts...>(
            std::forward<Func>(func),
            ptr(),
            tag() - 1uz
        );
    }

    template <typename Func>
    decltype(auto) call(Func &&func) const {
        return detail::tagged_ptr::dispatch_call<Func, Ts...>(
            std::forward<Func>(func),
            ptr(),
            tag() - 1uz
        );
    }

    auto tag() const {
        return static_cast<std::uint64_t>(tagged_address >> TAG_SHIFT);
    }

    auto ptr() const {
        return reinterpret_cast<const void*>(tagged_address & GET_PTR_MASK);
    }

    auto ptr() {
        return reinterpret_cast<void*>(tagged_address & GET_PTR_MASK);
    }

    static constexpr auto number_of_types() {
        return sizeof...(Ts);
    }
};

} // namespace zstl end
