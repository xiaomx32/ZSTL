#pragma once

namespace zstl {

template <typename T>
class complex {
public:
    T real {};
    T imag {};

    complex(T value)
        : real(value)
        , imag(0)
    {}

    complex(T real_, T imag_)
        : real(real_)
        , imag(imag_)
    {}

    complex operator-() const {
        return {
            -real,
            -imag
        };
    }

    complex operator+(complex z) const {
        return {
            real + z.real,
            imag + z.imag
        };
    }

    complex operator-(complex z) const {
        return {
            real - z.real,
            imag - z.imag
        };
    }

    complex operator*(complex z) const {
        return {
            real * z.real - imag * z.imag,
            real * z.imag + imag * z.real
        };
    }

    complex operator/(complex z) const {
        T scale = 1.0 / (z.real * z.real + z.imag * z.imag);
        return {
            scale * (real * z.real + imag * z.imag),
            scale * (imag * z.real - real * z.imag)
        };
    }

    friend complex operator+(T value, complex z) {
        return complex(value) + z;
    }

    friend complex operator-(T value, complex z) {
        return complex(value) - z;
    }

    friend complex operator*(T value, complex z) {
        return complex(value) * z;
    }

    friend complex operator/(T value, complex z) {
        return complex(value) / z;
    }
};

} // namespace zstl end
