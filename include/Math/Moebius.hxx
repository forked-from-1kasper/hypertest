#pragma once

#include <Math/Gyrovector.hxx>

// M = (az + b) / (cz + d)
template<typename T> struct Möbius {
    std::complex<T> a, b, c, d;

    constexpr Möbius() : a(1), b(0), c(0), d(1) {}
    constexpr Möbius(auto a, auto b, auto c, auto d) : a(a), b(b), c(c), d(d) {}

    constexpr std::complex<T> det() const { return a * d - b * c; }
    constexpr std::complex<T> tr()  const { return a + d; }
    constexpr std::complex<T> rot() const { return det() / Math::sqr(d); }

    constexpr inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    constexpr inline void normalize() { auto σ = sqrt(det()); a /= σ; b /= σ; c /= σ; d /= σ; }

    constexpr Gyrovector<T> apply(const Gyrovector<T> & w) const
    { return (a * w.val + b) / (c * w.val + d); }

    constexpr inline auto origin() const { return Gyrovector<T>(b / d); }

    constexpr inline Möbius<T> inverse() const { return Möbius<T>(d, -b, -c, a); }

    constexpr static inline Möbius<T> identity() { return Möbius<T>(1, 0, 0, 1); }

    constexpr static Möbius<T> translate(const Gyrovector<T> & N)
    { return Möbius<T>(1, N.val, Math::conjc(N.val), 1); }

    friend std::ostream & operator<< (std::ostream & stream, const Möbius<T> & M)
    { return stream << "(" << M.a << ", " << M.b << ", " << M.c << ", " << M.d << ")"; }
};

template<typename T> Möbius<T> operator*(const Möbius<T> & A, const Möbius<T> & B) {
    return {
        A.a * B.a + A.b * B.c,
        A.a * B.b + A.b * B.d,
        A.c * B.a + A.d * B.c,
        A.c * B.b + A.d * B.d
    };
}
