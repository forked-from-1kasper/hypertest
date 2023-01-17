#pragma once

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Gyrovector.hpp>

template<typename T>
struct Möbius {
    constexpr static auto zero = Gyrovector<T>(0, 0);

    std::complex<T> a, b, c, d;

    constexpr Möbius() {}
    constexpr Möbius(auto a, auto b, auto c, auto d) : a(a), b(b), c(c), d(d) {}

    constexpr std::complex<T> det() const { return a * d - b * c; }
    constexpr std::complex<T> tr()  const { return a + d; }
    constexpr std::complex<T> rot() const { return det() / Math::sqr(d); }

    constexpr inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    constexpr inline void normalize() { auto σ = sqrt(det()); a /= σ; b /= σ; c /= σ; d /= σ; }

    constexpr Gyrovector<T> apply(const Gyrovector<T> & w) const
    { return (a * w.val + b) / (c * w.val + d); }

    constexpr inline Gyrovector<T> origin() const { return apply(zero); }

    constexpr inline Möbius<T> inverse() const { return Möbius<T>(d, -b, -c, a); }

    constexpr static inline Möbius<T> identity() { return {1, 0, 0, 1}; }

    constexpr static Möbius<T> translate(const Gyrovector<T> & N)
    { return {1, N.val, std::conj(N.val), 1}; }

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

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Transform(const Möbius<T> & M)
{ return [M](Gyrovector<T> A) { return M.apply(A); }; }