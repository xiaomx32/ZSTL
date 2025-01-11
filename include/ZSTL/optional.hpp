#pragma once

#include <cstdint>
#include <ostream>
#include <exception>
#include <initializer_list>
#include <type_traits>


// ref: https://en.cppreference.com/w/cpp/utility/optional
namespace zstl {

// nullopt_t
// Tag type to disengage optional objects
struct nullopt_t {
    // Used for constructing nullopt
    enum class Construct : std::uint8_t { Token };

    constexpr explicit nullopt_t(Construct) noexcept {};
};

inline constexpr nullopt_t nullopt { nullopt_t::Construct::Token };


// bad_optional_access
class bad_optional_access : std::exception {
public:
    bad_optional_access() = default;
    virtual ~bad_optional_access() = default;

    const char* what() const noexcept override {
        return "bad optional access";
    }
};


// in_place_t
struct in_place_t {
    explicit in_place_t() = default;
};

inline constexpr in_place_t in_place {};




// optional
template <typename T>
class optional {
public:
    using value_type = T;

private:
    bool m_has_value { false };
    union {
        std::aligned_storage_t<sizeof(T), alignof(T)> m_value;
    };

public:
    constexpr optional() noexcept
        : m_has_value(false)
    {}

    constexpr optional(nullopt_t) noexcept
        : m_has_value(false)
    {}

    constexpr optional(const optional &other)
        : m_has_value(other.has_value())
    {
        if (m_has_value) {
            // placement-new
            new (ptr()) T(other.value());
        }
    }

    constexpr optional(optional&& other) noexcept
        : m_has_value(other.has_value())
    {
        if (m_has_value) {
            new (ptr()) T(std::move(other.value()));
            other.reset();
        }
    }

    template <typename U>
    constexpr optional(const optional<U>& other) {

    }

    template <typename U>
    constexpr optional(const optional<U>&& other) {

    }

    template <typename... Args>
    constexpr explicit optional(in_place_t, Args&&... args)
        : m_has_value(true)
        , m_value(std::forward<Args>(args)...)
    {}

    template <typename U, typename... Args >
    constexpr explicit optional(
        in_place_t,
        std::initializer_list<U> ilist,
        Args&&... args
    )
        : m_has_value(true)
        , m_value(ilist, std::forward<Args>(args)...)
    {};

    template <typename U = T >
    constexpr optional(U&& value)
        : m_has_value(true)
    {
        new (ptr()) U(std::move(value));
    }

    ~optional() {
        reset();
    }


    optional& operator=(nullopt_t) noexcept {
        if (m_has_value) {
            m_value.~T();
            m_has_value = false;
        }

        return *this;
    }

    constexpr optional& operator=(const optional& other) {
        reset();
        if (other.has_value()) {
            new (ptr()) T(other.value());
            m_has_value = true;
        }

        return *this;
    }

    constexpr optional& operator=(optional&& other) {
        reset();
        if (other.has_value()) {
            new (ptr()) T(std::move(other.value()));
            m_has_value = true;
            other.reset();
        }

        return *this;
    }

    template <typename U = T>
    constexpr optional& operator=(U&& v) {
        reset();
        new (ptr()) T(std::move(v));
        m_has_value = true;

        return *this;
    }

    template <typename U >
    optional& operator=(const optional<U>& other) {
        reset();
        if (other.has_value()) {
            new (ptr()) T(other.value());
            m_has_value = true;
        }

        return *this;
    }

    template <typename U>
    optional& operator=(optional<U>&& other) {
        reset();
        new (ptr()) T(std::move(other));
        m_has_value = true;

        return *this;
    }

    constexpr const T* operator->() const noexcept {
        return &value;
    }

    constexpr T *operator->() noexcept {
        return &value;
    }

    // constexpr const T& operator*() const& noexcept;

    // constexpr T& operator*() & noexcept;
    // constexpr const T&& operator*() const&& noexcept;

    // constexpr T&& operator*() && noexcept;

    // constexpr const T* operator->() const noexcept;

    // constexpr T* operator->() noexcept;

    constexpr const T& operator*() const& noexcept {
        return m_value;
    }

    constexpr T& operator*() & noexcept {
        return m_value;
    }

    constexpr const T&& operator*() const&& noexcept {
        return std::move(m_value);
    }

    constexpr T&& operator*() && noexcept {
        return std::move(m_value);
    }

    constexpr explicit operator bool() const noexcept {
        return m_has_value;
    }

    bool has_value() const noexcept {
        return m_has_value;
    }

    constexpr T& value() & {
        CHECK(m_has_value);
        return *ptr();
    }

    constexpr const T& value() const& {
        CHECK(m_has_value);
        return *ptr();
    }

    // constexpr T&& value() &&;

    // constexpr const T&& value() const&&;

    template <typename U>
    constexpr T value_or(U&& default_value) const& {
        return m_has_value
            ? m_value
            : default_value;
    }

    template <typename U>
    constexpr T value_or(U&& default_value) && {
        return m_has_value
            ? std::move(m_value)
            : default_value;
    };

    void swap(optional &other) noexcept {
        if (m_has_value && other.m_has_value) {
            using std::swap; // ADL
            swap(m_value, other.m_value);
        } else if (!m_has_value && !other.m_has_value) {
            // do nothing
        } else if (m_has_value) {
            other.emplace(std::move(m_value));
            reset();
        } else {
            emplace(std::move(other.m_value));
            other.reset();
        }
    }

    constexpr void reset() noexcept { // Equivalent to *this = nullopt;
        if (m_has_value) {
            m_value.~T();
            m_has_value = false;
        }
    }

    template <typename... Args>
    T& emplace(Args&&... args) {
        if (m_has_value) {
            m_value.~T();
            m_has_value = false;
        }
        new (&m_value) T(std::forward<Args>(args)...);
        m_has_value = true;
    }

    template <typename U, class... Args>
    T& emplace( std::initializer_list<U> ilist, Args&&... args) {
        if (m_has_value) {
            m_value.~T();
            m_has_value = false;
        }
        new (&m_value) T(ilist, std::forward<Args>(args)...);
        m_has_value = true;
    }

private:
#ifdef __NVCC__
    // Work-around NVCC bug
    T *ptr() {
        return reinterpret_cast<T *>(&value);
    }

    const T *ptr() const {
        return reinterpret_cast<const T *>(&value);
    }
#else
    T *ptr() {
        return std::launder(
            reinterpret_cast<T *>(&value)
        );
    }

    const T *ptr() const {
        return std::launder(
            reinterpret_cast<const T *>(&value)
        );
    }
#endif
};

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const optional<T> &opt) {
    if (opt.has_value()) {
        return os << "[ pstd::optional<"
                << typeid(T).name()
                << "> has_value: true "
                << "value: " << opt.value()
                << " ]";
    } else {
        return os << "[ pstd::optional<" << typeid(T).name()
                << "> has_value: false value: n/a ]";
    }
}

} // namespace zstl end
