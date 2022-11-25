#include "Matrix.hpp"
#include <numeric>

#pragma once

constexpr double s²  = 6;
constexpr double s⁻² = 1/s²;
constexpr double s   = sqrt(s²);

template<typename T>
struct Gyrovector {
    std::complex<T> val;

    inline T x() const { return val.real(); }
    inline T y() const { return val.imag(); }

    constexpr inline T norm() const { return std::norm(val); }
    constexpr inline T abs() const  { return std::abs(val); }
    constexpr inline bool isZero() const { return val.real() == 0.0 && val.imag() == 0.0; }

    constexpr Gyrovector(const T k) : val(std::complex(k, 0.0)) {}
    constexpr Gyrovector(const T x, const T y) : val(std::complex(x, y)) {}
    constexpr Gyrovector(const std::complex<T> z) : val(z) {}

    constexpr T operator,(const Gyrovector<T> & N) { return val.real() * N.val.real() + val.imag() * N.val.imag(); }

    constexpr inline auto add(const Gyrovector<T> & N) const { return Gyrovector<T>(val + N.val); }
    constexpr inline auto conj() const { return Gyrovector<T>(std::conj(val)); }
    constexpr inline auto scale(const T k) const { return Gyrovector<T>(k * val); }
    constexpr inline auto mult(const Gyrovector<T> & N) const { return Gyrovector<T>(val * N.val); }
    constexpr inline auto inv() const { return Gyrovector<T>(1.0 / val); }
    constexpr inline auto div(const Gyrovector<T> & N) const { return Gyrovector<T>(val / N.val); }

    constexpr inline auto operator-() const { return Gyrovector<T>(-val); }
    constexpr inline auto operator+() const { return *this; }

    constexpr Vector3<T> elevate(const T & z) { return vector(x(), z, y()); }
};

template<typename T> constexpr auto operator+(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return A.add(B).div(A.conj().mult(B).scale(s⁻²).add(1)); }

template<typename T> constexpr auto operator*(const T k, const Gyrovector<T> & A)
{ auto a = A.abs(); return A.isZero() ? A : A.scale(s * tanh(k * atanh(a / s)) / a); }

template<typename T> constexpr Gyrovector<T> gyr(const Gyrovector<T> & A, const Gyrovector<T> & B, const Gyrovector<T> & C) {
    auto P = A.mult(B.conj()).add(s²);
    auto Q = A.conj().mult(B).add(s²);
    return P.div(Q).mult(C);
}

template<typename T> constexpr Gyrovector<T> Coadd(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto a = A.norm() / s²; auto b = B.norm() / s²;
    return A.scale(1 - b).add(B.scale(1 - a)).scale(1 / (1 - a * b));
}

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Gyr(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return [A, B](Gyrovector<T> C) { return gyr(A, B, C); }; }

template<typename T> std::function<Gyrovector<T>(T)> Line(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ auto N = (-A) + B; return [A, N](T t) { return A + (t * N); }; }

template<typename T> constexpr Gyrovector<T> midpoint(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return A + 0.5 * (-A + B); }

template<typename T> std::function<Gyrovector<T>(T)> Coline(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ auto N = Coadd(B, -A); return [A, N](T t) { return (t * N) + A; }; }

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Translate(const Gyrovector<T> & N)
{ return [N](Gyrovector<T> A) { return N + A; }; }

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Scalar(const T k)
{ return [k](Gyrovector<T> A) { return k * A; }; }

template<typename T> T holonomy(const Gyrovector<T> & P₁, const Gyrovector<T> & P₂) {
    const auto n₁ = Gyrovector<double>(0.0, 1.0);
    const auto n₂ = gyr(P₂, -P₁, n₁);
    return std::arg(n₁.val / n₂.val);
}

namespace complex {
    template <typename T, typename U>
    constexpr inline std::complex<U> cast(const std::complex<T> & z)
    { return std::complex<U>(z.real(), z.imag()); }
}

template<typename T>
struct Möbius {
    std::complex<T> a, b, c, d;

    constexpr std::complex<T> det() const { return a * d - b * c; }

    constexpr inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    constexpr inline Möbius<T> normalize() const { return div(det()); }

    constexpr Gyrovector<T> apply(const Gyrovector<T> & w) const
    { return (a * w.val + b) / (c * w.val + d); }

    constexpr inline Gyrovector<T> origin() const
    { const auto O = Gyrovector<T>(0, 0); return apply(O); }

    constexpr inline Möbius<T> inverse() const { return Möbius<T>(d, -b, -c, a).normalize(); }

    constexpr static inline Möbius<T> identity() { return {1, 0, 0, 1}; }

    constexpr static Möbius<T> translate(const Gyrovector<T> & N)
    { return {s², s² * N.val, std::conj(N.val), s²}; }

    void simpl() {
        auto σ = a.real();

        σ = std::gcd(b.real(), σ);
        σ = std::gcd(c.real(), σ);
        σ = std::gcd(d.real(), σ);
        σ = std::gcd(a.imag(), σ);
        σ = std::gcd(b.imag(), σ);
        σ = std::gcd(c.imag(), σ);
        σ = std::gcd(d.imag(), σ);

        a /= σ; b /= σ; c /= σ; d /= σ;
    }

    template<typename U> constexpr inline Möbius<U> cast() const {
        return {
            complex::cast<T, U>(a), complex::cast<T, U>(b),
            complex::cast<T, U>(c), complex::cast<T, U>(d)
        };
    }
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
