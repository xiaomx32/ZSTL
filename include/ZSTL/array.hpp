#pragma once

#include <cstddef> // std::size_t
#include <stdexcept> // std::out_of_range
#include <cassert> // assert
#include <iterator> // std::reverse_iterator
#include <algorithm> // std::equal
#include <initializer_list>


namespace zero {

// ref: https://en.cppreference.com/w/cpp/container/array
namespace zstd {

template <typename _Tp, std::size_t _N>
class array {
private:
    _Tp _M_values[_N] = {};

public:
    using value_type = _Tp;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = value_type *;
    using const_iterator = const value_type *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    array() = default;

    array(std::initializer_list<_Tp> v) {
        std::size_t i { 0 };
        for (const_reference val : v) {
            _M_values[i++] = val;
        }
    }

    // Element access
    constexpr reference at(size_type i) {
        if (i >= _N) [[__unlikely__]] {
            throw std::out_of_range("zero::zstd::array::at");
        }

        return _M_values[i];
    }

    constexpr const_reference at(size_type i) const {
        if (i < 0 || i >= _N) [[__unlikely__]] {
            throw std::out_of_range("zero::zstd::array::at");
        }

        return _M_values[i];
    }

    constexpr reference operator[](size_type i) {
        return _M_values[i];
    }

    constexpr const_reference operator[](size_type i) const {
        return _M_values[i];
    }

    constexpr reference front() {
        return _M_values[0];
    }

    constexpr const_reference front() const {
        return _M_values[0];
    }

    constexpr reference back() {
        return _M_values[_N - 1];
    }

    constexpr const_reference back() const {
        return _M_values[_N - 1];
    }

    constexpr pointer data() {
        return _M_values;
    }

    constexpr const_pointer data() const {
        return _M_values;
    }

    // Iterators
    constexpr iterator begin() noexcept {
        return _M_values;
    }

    constexpr const_iterator begin() const noexcept {
        return _M_values;
    }

    constexpr const_iterator cbegin() const noexcept {
        return _M_values;
    }

    constexpr iterator end() noexcept {
        return _M_values + _N;
    }

    constexpr const_iterator end() const noexcept {
        return _M_values + _N;
    }

    constexpr const_iterator cend() const noexcept {
        return _M_values + _N;
    }

    constexpr reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(_M_values + _N);
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(_M_values + _N);
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(_M_values + _N);
    }

    // Capacity
    [[nodiscard]] constexpr bool empty() const noexcept {
        return false;
    }

    constexpr size_type size() const noexcept {
        return _N;
    }

    constexpr size_type max_size() const noexcept {
        return _N;
    }

    // Operations
    void fill(const_reference v) {
        for (size_type i = 0; i < _N; ++i) {
            _M_values[i] = v;
        }
    }

    void swap(array &other) noexcept {
        for (size_type i = 0; i < _N; ++i) {
            std::swap(_M_values[i], other._M_values[i]);
        }
    }

    bool operator==(const array<_Tp, _N> &a) const {
        for (size_type i = 0; i < _N; ++i) {
            if (_M_values[i] != a._M_values[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const array<_Tp, _N> &a) const {
        return !(*this == a);
    }
};
template <typename _Tp, std::size_t _N>
bool operator==(
    const array<_Tp, _N> &lhs,
    const array<_Tp, _N> &rhs
) noexcept {
    return std::equal(
        lhs->begin(), lhs->end(),
        rhs.begin(), rhs.end()
    );
}

template <typename _Tp, std::size_t _N>
auto operator<=>(
    const array<_Tp, _N> &lhs,
    const array<_Tp, _N> &rhs
) noexcept {
    return std::lexicographical_compare_three_way(
        lhs->begin(), lhs->end(),
        rhs.begin(), rhs.end()
    );
}


// Specialization for zero element arrays (to make MSVC happy)
template <typename _Tp>
class array<_Tp, 0> {
public:
    using value_type = _Tp;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = value_type *;
    using const_iterator = const value_type *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    array() = default;

    // Element access
    constexpr reference at(size_type i) {
        throw std::out_of_range("zero::zstd::array::at");
    }

    constexpr const_reference at(size_type i) const {
        throw std::out_of_range("zero::zstd::array::at");
    }

    constexpr reference operator[](size_type i) {
        assert(!"should never be called");
        static _Tp t {};
        return t;
    }

    constexpr const_reference operator[](size_type i) const {
        assert(!"should never be called");
        static _Tp t {};
        return t;
    }

    constexpr reference front() {
        __builtin_unreachable();
    }

    constexpr const_reference front() const {
        __builtin_unreachable();
    }

    constexpr reference back() {
        __builtin_unreachable();
    }

    constexpr const_reference back() const {
        __builtin_unreachable();
    }

    constexpr pointer data() {
        return nullptr;
    }

    constexpr const_pointer data() const {
        return nullptr;
    }

    // Iterators
    constexpr iterator begin() noexcept {
        return nullptr;
    }

    constexpr const_iterator begin() const noexcept {
        return nullptr;
    }

    constexpr const_iterator cbegin() const noexcept {
        return nullptr;
    }

    constexpr iterator end() noexcept {
        return nullptr;
    }

    constexpr const_iterator end() const noexcept {
        return nullptr;
    }

    constexpr const_iterator cend() const noexcept {
        return nullptr;
    }

    constexpr reverse_iterator rend() noexcept {
        return nullptr;
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return nullptr;
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return nullptr;
    }

    // Capacity
    [[nodiscard]] constexpr bool empty() const noexcept {
        return true;
    }

    constexpr size_type size() const noexcept {
        return 0;
    }

    constexpr size_type max_size() const noexcept {
        return 0;
    }

    // Operations
    void fill(const_reference v) {
        assert(!"should never be called");
    }

    void swap(array &other) noexcept {
        assert(!"should never be called");
    }

    bool operator==(const array<_Tp, 0> &a) const {
        return true;
    }

    bool operator!=(const array<_Tp, 0> &a) const {
        return false;
    }
};

// deduction guides for std::array
// ref: https://en.cppreference.com/w/cpp/container/array/deduction_guides
template< class T, class... U >
array(T, U...) -> array<T, 1 + sizeof...(U)>;

} // namespace zstd end

} // namespace zero end
