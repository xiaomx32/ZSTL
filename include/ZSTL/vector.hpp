#pragma once

#include "memory_resource.hpp"

#include <limits>
#include <cstddef>
#include <iterator>
#include <initializer_list>


// ref: https://en.cppreference.com/w/cpp/container/vector/vector
namespace zstl {

template <
    typename _Tp,
    class Allocator = pmr::polymorphic_allocator<_Tp>
>
class vector {
public:
    using value_type = _Tp;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = _Tp &;
    using const_reference = const _Tp &;
    using pointer = _Tp *;
    using const_pointer = const _Tp *;
    using iterator = _Tp *;
    using const_iterator = const _Tp *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const iterator>;

private:
    allocator_type alloc;
    pointer ptr { nullptr };
    size_type nAlloc { 0uz };
    size_type nStored { 0uz };

public:
    // Constructor
    constexpr vector() noexcept(noexcept(allocator_type()))
        : ptr { nullptr }
        , nAlloc { 0uz }
        , nStored { 0uz }
    {}

    constexpr explicit vector(const allocator_type &alloc = {}) noexcept
        : alloc(alloc)
    {}

    constexpr vector(
        size_type count,
        const_reference value,
        const allocator_type &alloc = allocator_type()
    )
        : alloc(alloc)
    {
        this->reserve(count);
        for (size_type i { 0uz }; i < count; ++i) {
            this->alloc.template construct<value_type>(
                this->ptr + i,
                value
            );
        }
        this->nStored = count;
    }

    explicit vector(
        size_type count,
        const allocator_type &alloc = allocator_type()
    )
        : vector(count, value_type {}, alloc)
    {}

    // InputIt: https://en.cppreference.com/w/cpp/named_req/InputIterator
    template <class InputIt>
    constexpr vector(
        InputIt first,
        InputIt last,
        const allocator_type &alloc = allocator_type()
    )
        : alloc(alloc)
    {
        size_type size = last - first;
        reserve(size);
        size_type i { 0 };
        for (InputIt iter = first; iter != last; ++iter, ++i) {
            this->alloc.template construct<value_type>(
                this->ptr + i,
                *iter
            );
        }
        this->nStored = size;
    }

    constexpr vector(const vector &other)
        : alloc(other.alloc)
        , nAlloc(other.nAlloc)
        , nStored(other.nStored)
    {
        if (this->nStored != 0uz) {
            this->ptr = this->alloc.template allocate_object<value_type>(this->nStored);
            for (size_type i { 0uz }; i < this->nStored; ++i) {
                this->alloc.template construct<value_type>(
                    this->ptr + i,
                    other[i]
                );
            }
        } else {
            this->ptr = nullptr;
        }
    }

    constexpr vector(
        const vector &other,
        const allocator_type &alloc = allocator_type()
    )
        : alloc(alloc)
    {
        this->reserve(other.size());
        for (size_type i { 0uz }; i < other.size(); ++i) {
            this->alloc.template construct<value_type>(
                this->ptr + i,
                other[i]
            );
        }
        this->nStored = other.size();
    }

    constexpr vector(vector &&other) noexcept
        : alloc(std::move(other.alloc))
    {
        this->ptr = other.ptr;
        this->nAlloc = other.nAlloc;
        this->nStored = other.nStored;

        other.nStored = 0uz;
        other.nAlloc = 0uz;
        other.ptr = nullptr;
    }

    constexpr vector(
        vector &&other,
        const allocator_type &alloc
    )
        : alloc(alloc)
    {
        // if (alloc == other.alloc) {
            this->ptr = other.ptr;
            this->nAlloc = other.nAlloc;
            this->nStored = other.nStored;

            other.ptr = nullptr;
            other.nAlloc = 0uz;
            other.nStored = 0uz;
        // } else {
        //     reserve(other.size());
        //     for (size_type i { 0 }; i < other.size(); ++i) {
        //         alloc.template construct<value_Type>(this->ptr + i, std::move(other[i]));
        //     }
        //     this->nStored = other.size();
        // }
    }

    constexpr vector(
        std::initializer_list<value_type> init,
        const allocator_type &alloc = allocator_type()
    )
        : vector(init.begin(), init.end(), alloc)
    {}

    constexpr vector &operator=(const vector &other) {
        if (this == &other) [[unlikely]] {
            return *this;
        }

        this->clear();
        this->reserve(other.size());
        for (size_type i { 0uz }; i < other.size(); ++i) {
            this->alloc.template construct<value_type>(
                this->ptr + i,
                other[i]
            );
        }
        this->nStored = other.size();

        return *this;
    }

    vector &operator=(vector &&other) noexcept {
        if (this == &other) [[unlikely]] {
            return *this;
        }

        if (this->alloc == other.alloc) {
            zstd::swap(this->ptr, other.ptr);
            zstd::swap(this->nAlloc, other.nAlloc);
            zstd::swap(this->nStored, other.nStored);
        } else {
            clear();
            reserve(other.size());
            for (size_type i { 0uz }; i < other.size(); ++i) {
                this->alloc.template construct<value_type>(
                    this->ptr + i,
                    std::move(other[i])
                );
            }
            this->nStored = other.size();
        }

        return *this;
    }

    constexpr vector &operator=(std::initializer_list<value_type> &init) {
        reserve(init.size());
        clear();
        iterator iter = begin();
        for (const value_type &value : init) {
            *iter = value;
            ++iter;
        }

        return *this;
    }

    constexpr void assign(
        size_type count,
        const value_type &value
    ) {
        this->clear();
        this->reserve(count);
        for (size_type i { 0uz }; i < count; ++i) {
            this->push_back(value);
        }
    }

    template <class InputIt>
    constexpr void assign(
        InputIt first,
        InputIt last
    ) {
        std::size_t n = last - first;
        this->clear();
        this->reserve(n);
        this->nStored = n;
        for (size_type i { 0uz }; i != n; ++i) {
            this->alloc.template construct<value_type>(
                this->ptr + i,
                *first
            );
            ++first;
        }
    }

    constexpr void assign(std::initializer_list<value_type> &init) {
        this->assign(init.begin(), init.end());
    }

    constexpr allocator_type get_allocator() const noexcept {
        return this->alloc;
    }

    // Destructor
    constexpr ~vector() {
        this->clear();
        this->alloc.deallocate_object(this->ptr, this->nAlloc);
    }

    // Element access
    reference at(size_type index) {
        if (index >= this->nStored) [[unlikely]] {
            throw std::out_of_range("vector::at");
        }

        return this->ptr[index];
    }

    const_reference at(size_type index) const {
        if (index >= this->nStored) [[unlikely]] {
            throw std::out_of_range("vector::at");
        }

        return this->ptr[index];
    }

    constexpr reference operator[](size_type index) {
        // DCHECK_LT(index, this->nStored);
        return this->ptr[index];
    }

    constexpr const_reference operator[](size_type index) const {
        // DCHECK_LT(index, this->nStored);
        return this->ptr[index];
    }

    constexpr reference front() noexcept {
        return this->ptr[0uz];
    }

    constexpr const_reference front() const noexcept {
        return this->ptr[0uz];
    }

    constexpr reference back() noexcept {
        return this->ptr[nStored - 1uz];
    }

    constexpr const_reference back() const noexcept {
        return this->ptr[nStored - 1uz];
    }

    constexpr pointer data() noexcept {
        return this->ptr;
    }

    constexpr const_pointer data() const noexcept {
        return this->ptr;
    }

    // Iterators
    constexpr iterator begin() noexcept {
        return this->ptr;
    }

    constexpr const_iterator begin() const noexcept {
        return this->ptr;
    }

    constexpr const_iterator cbegin() const noexcept {
        return this->ptr;
    }

    constexpr iterator end() noexcept {
        return this->ptr + this->nStored;
    }

    constexpr const_iterator end() const noexcept {
        return this->ptr + this->nStored;
    }

    constexpr const_iterator cend() const noexcept {
        return this->ptr + this->nStored;
    }

    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(this->ptr + this->nStored);
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(this->ptr + this->nStored);
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(this->ptr + this->nStored);
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(this->ptr);
    }

    constexpr const_reverse_iterator rend() const {
        return const_reverse_iterator(this->ptr);
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(this->ptr);
    }

    // Capacity(TODO: shrink_to_fit)
    constexpr bool empty() const noexcept {
        return this->nStored == 0uz;
    }

    constexpr size_type size() const noexcept {
        return this->nStored;
    }

    // https://en.cppreference.com/w/cpp/container/vector/max_size
    constexpr size_type max_size() const noexcept {
        // return (size_type) - 1;
        // return std::numeric_limits<difference_type>::max();
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    constexpr void reserve(size_type n) {
        if (n <= this->nAlloc) { return ; }

        value_type *ra = this->alloc.template allocate_object<value_type>(n);
        for (size_type i { 0uz }; i < nStored; ++i) {
            this->alloc.template construct<value_type>(ra + i, std::move(this->begin()[i]));
            this->alloc.destroy(this->begin() + i);
        }

        this->alloc.deallocate_object(ptr, nAlloc);
        this->nAlloc = n;
        this->ptr = ra;
    }

    constexpr size_type capacity() const noexcept {
        return this->nAlloc;
    }

    constexpr void shrink_to_fit() {
        // auto __old_data = _M_data;
        // auto __old_cap = _M_cap;
        // _M_cap = _M_size;
        // if (_M_size == 0) {
        //     _M_data = nullptr;
        // } else {
        //     _M_data = _M_alloc.allocate(_M_size);
        // }
        // if (__old_cap != 0) [[likely]] {
        //     for (std::size_t __i = 0; __i != _M_size; __i++) {
        //         std::construct_at(&_M_data[__i], std::move_if_noexcept(__old_data[__i])); // _M_data[__i] = std::move(__old_data[__i])
        //         std::destroy_at(&__old_data[__i]);
        //     }
        //     _M_alloc.deallocate(__old_data, __old_cap);
        // }
    }

    // Modifiers
    void clear() noexcept {
        for (size_type i { 0uz }; i < this->nStored; ++i) {
            this->alloc.destroy(&ptr[i]);
        }
        this->nStored = 0uz;
    }

    constexpr iterator insert(
        const_iterator,
        const_reference value
    ) {
        // TODO
        log::log_fatal("TODO");
    }

    constexpr iterator insert(
        const_iterator,
        value_type &&value
    ) {
        // TODO
        log::log_fatal("TODO");
    }

    constexpr iterator insert(
        const_iterator pos,
        size_type count,
        const_reference value
    ) {
        // TODO
        log::log_fatal("TODO");
    }

    template <class InputIt>
    constexpr iterator insert(
        const_iterator pos,
        InputIt first,
        InputIt last
    ) {
        if (pos == end()) {
            size_type firstOffset = size();
            for (auto iter = first; iter != last; ++iter) {
                push_back(*iter);
            }
            return begin() + firstOffset;
        } else {
            log::log_fatal("TODO");
        }
    }

    constexpr iterator insert(
        const_iterator pos,
        std::initializer_list<value_type> init
    ) {
        // TODO
        log::log_fatal("TODO");
    }

    template <class... Args>
    constexpr iterator emplace(const_iterator pos, Args &&...args) {
        // TODO
        log::log_fatal("TODO");
    }

    constexpr iterator erase(const_iterator pos) {
        // TODO
        log::log_fatal("TODO");
    }

    constexpr iterator erase(const_iterator first, const_iterator last) {
        // TODO
        log::log_fatal("TODO");
    }

    constexpr void push_back(const_reference value) {
        if (nAlloc == nStored) {
            reserve(nAlloc == 0uz ? 4uz : 2uz * nAlloc);
        }

        alloc.construct(ptr + nStored, value);
        ++nStored;
    }

    constexpr void push_back(value_type &&value) {
        if (nAlloc == nStored) {
            reserve(nAlloc == 0uz ? 4uz : 2uz * nAlloc);
        }

        this->alloc.construct(ptr + nStored, std::move(value));
        ++nStored;
    }

    // For C++17
    // template <class... Args>
    // void emplace_back(Args &&...args) {
    //     if (nAlloc == nStored) {
    //         reserve(nAlloc == 0 ? 4 : 2 * nAlloc);
    //     }

    //     alloc.construct(ptr + nStored, std::forward<Args>(args)...);
    //     ++nStored;
    // }

    template <class... Args>
    constexpr reference emplace_back(Args &&...args) {
        if (this->nStored == this->nAlloc) {
            reserve(this->nAlloc == 0uz ? 4uz : 2uz * this->nAlloc);
        }

        pointer _p = &this->ptr[this->nStored];
        alloc.construct(_p, std::forward<Args>(args)...);
        ++(this->nStored);

        return *_p;
    }

    constexpr void pop_back() {
        // DCHECK(!empty());
        this->alloc.destroy(this->ptr + this->nStored - 1uz);
        --(this->nStored);
    }

    constexpr void resize(size_type count) {
        if (count < this->nStored) {
            for (size_type i = count; i < this->nStored; ++i) {
                this->alloc.destroy(ptr + i);
            }
            if (count == 0uz) {
                this->alloc.deallocate_object(ptr, nAlloc);
                this->ptr = nullptr;
                this->nAlloc = 0uz;
            }
        } else if (count > this->nStored) {
            this->reserve(count);
            for (size_type i = this->nStored; i < count; ++i) {
                this->alloc.construct(ptr + i);
            }
        }
        this->nStored = count;
    }

    constexpr void resize(size_type count, const_reference value) {
        if (count < this->nStored) {
            for (size_type i = count; i != this->nStored; ++i) {
                // std::destroy_at(&this->ptr[i]);
                this->alloc.destroy(ptr + i);
            }
            this->nStored = count;
        } else if (count > this->nStored) {
            this->reserve(count);
            for (size_type i = this->nStored; i != count; ++i) {
                // std::construct_at(&this->ptr[i], value); // this->ptr[i] = value
                this->alloc.construct(ptr + nStored, value);
            }
        }
        this->nStored = count;
    }

    constexpr void swap(vector &other) noexcept {
        // CHECK(alloc == other.alloc);  // TODO: handle this
        std::swap(this->alloc, other.alloc);
        std::swap(this->ptr, other.ptr);
        std::swap(this->nAlloc, other.nAlloc);
        std::swap(this->nStored, other.nStored);
    }
};

} // namespace zstl end
