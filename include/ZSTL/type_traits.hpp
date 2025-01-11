#pragma once

#include <type_traits> // std::is_same_v


namespace zstl {

// contain_type
// determine whether the template parameter Ts contains type T
template <typename T, typename... Ts>
concept contain_type = (std::is_same_v<T, Ts> || ...);


// type_index
// calculate the index of type T in template parameter Ts (counting from 0)
template <typename T, typename... Ts>
struct type_index;

template <typename T0, typename... Ts>
struct type_index<T0, T0, Ts...>
    : public std::integral_constant<std::size_t, 0uz>
{};

template <typename T, typename T0, typename... Ts>
struct type_index<T, T0, Ts...>
    : public std::integral_constant<
        std::size_t,
        type_index<T, Ts...>::value + 1uz
    >
{};

template <typename T, typename... Ts>
inline constexpr std::size_t type_index_v = type_index<T, Ts...>::value;

} // namespace zstl end
