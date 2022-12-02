#include "Gyrovector.hpp"
#include <gmpxx.h>

#pragma once

template<typename T> T hcf(T &, T &) = delete;

inline auto hcf(int16_t & n, int16_t & m) { return std::gcd(n, m); }
inline auto hcf(int32_t & n, int32_t & m) { return std::gcd(n, m); }
inline auto hcf(int64_t & n, int64_t & m) { return std::gcd(n, m); }
inline auto hcf(mpz_class & n, mpz_class & m) { return gcd(n, m); }

template<typename T> concept EuclideanDomain =
requires(T a, T b) {
    {    -a     } -> std::convertible_to<T>;
    {   a + b   } -> std::convertible_to<T>;
    {   a - b   } -> std::convertible_to<T>;
    {   a * b   } -> std::convertible_to<T>;
    {   a / b   } -> std::convertible_to<T>;
    { hcf(a, b) } -> std::convertible_to<T>;
};

template<EuclideanDomain T>
struct Gaussian {
    T real, imag;

    constexpr inline auto operator-() const { return Gaussian(-real, -imag); }
    constexpr inline auto operator+() const { return *this; }

    auto operator+(const Gaussian<T> & w) const
    { return Gaussian<T>(real + w.real, imag + w.imag); }

    auto operator+=(const Gaussian<T> & w)
    { real += w.real; imag += w.imag; return *this; }

    auto operator-(const Gaussian<T> & w) const
    { return Gaussian<T>(real - w.real, imag - w.imag); }

    auto operator-=(const Gaussian<T> & w)
    { real -= w.real; imag -= w.imag; return *this; }

    auto operator*(const Gaussian<T> & w) const
    { return Gaussian<T>(real * w.real - imag * w.imag, real * w.imag + imag * w.real); }

    auto operator*=(const Gaussian<T> & w) {
        auto α(real), β(w.real);
        real = α * β - imag * w.imag;
        imag = α * w.imag + imag * β;
        return *this;
    }

    auto operator/(const T & k) const { return Gaussian<T>(real / k, imag / k); }
    auto operator/=(const T & k) { real /= k; imag /= k; return *this; }

    constexpr auto operator==(const Gaussian<T> & w) const
    { return real == w.real && imag == w.imag; }

    inline void negate() { real = -real; imag = -imag; }

    template<typename U> auto field() const { return std::complex<U>(real, imag); }
    template<EuclideanDomain U> auto transform() const { return Gaussian<U>(real, imag); }

    friend std::ostream & operator<< (std::ostream & stream, const Gaussian<T> & z)
    { return stream << z.real << " + " << z.imag << "i"; }
};

template<EuclideanDomain T>
struct Fuchsian {
    static constexpr auto s = sqrt(6);
    Gaussian<T> a, b, c, d;

    constexpr Gaussian<T> det() const { return a * d - b * c; }

    void simpl() {
        auto σ = a.real;

        σ = hcf(b.real, σ);
        σ = hcf(c.real, σ);
        σ = hcf(d.real, σ);
        σ = hcf(a.imag, σ);
        σ = hcf(b.imag, σ);
        σ = hcf(c.imag, σ);
        σ = hcf(d.imag, σ);

        a /= σ; b /= σ; c /= σ; d /= σ;

        if (a.real < 0) { a.negate(); b.negate(); c.negate(); d.negate(); }
    }

    template<typename U> constexpr inline Möbius<U> field() const {
        return { a.template field<U>(),     b.template field<U>() / s,
                 c.template field<U>() * s, d.template field<U>() };
    }

    constexpr inline auto inverse() const { return Fuchsian<T>(d, -b, -c, a); }

    constexpr auto operator==(const Fuchsian<T> & G) const
    { return a == G.a && b == G.b && c == G.b && d == G.d; }

    friend std::ostream & operator<< (std::ostream & stream, const Fuchsian<T> & G)
    { return stream << "(" << G.a << ", " << G.b << ", " << G.c << ", " << G.d << ")"; }
};

template<typename T> Fuchsian<T> operator*(const Fuchsian<T> & A, const Fuchsian<T> & B) {
    return {
        A.a * B.a + A.b * B.c,
        A.a * B.b + A.b * B.d,
        A.c * B.a + A.d * B.c,
        A.c * B.b + A.d * B.d
    };
}

template<typename T> void operator*=(Fuchsian<T> & A, const Fuchsian<T> & B) { A = A * B; A.simpl(); }