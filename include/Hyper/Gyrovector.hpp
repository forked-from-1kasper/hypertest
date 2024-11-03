#pragma once

#include <type_traits>
#include <functional>

#include <Math.hpp>

template<typename T>
struct Gyrovector {
    std::complex<T> val;

    constexpr inline T x() const { return val.real(); }
    constexpr inline T y() const { return val.imag(); }

    constexpr inline bool isZero() const { return val.real() == 0.0 && val.imag() == 0.0; }

    constexpr Gyrovector() : val(0) {}
    constexpr Gyrovector(const T k) : val(std::complex(k, 0.0)) {}
    constexpr Gyrovector(const T x, const T y) : val(std::complex(x, y)) {}
    constexpr Gyrovector(const std::complex<T> z) : val(z) {}

    template<typename U> constexpr operator Gyrovector<U>() const
    { return Gyrovector<U>(x(), y()); }

    constexpr T operator,(const Gyrovector<T> & N) const { return val.real() * N.val.real() + val.imag() * N.val.imag(); }

    constexpr inline T cross(const Gyrovector<T> & N) const { return x() * N.y() - y() * N.x(); }

    constexpr inline T abs()  const { return Math::absc(val); }
    constexpr inline T norm() const { return Math::normc(val); }

    constexpr inline auto add(const Gyrovector<T> & N) const { return Gyrovector<T>(Math::addc(val, N.val)); }
    constexpr inline auto sub(const Gyrovector<T> & N) const { return Gyrovector<T>(Math::subc(val, N.val)); }
    constexpr inline auto mul(const Gyrovector<T> & N) const { return Gyrovector<T>(Math::mulc(val, N.val)); }
    constexpr inline auto div(const Gyrovector<T> & N) const { return Gyrovector<T>(Math::divc(val, N.val)); }

    constexpr inline auto conj() const { return Gyrovector<T>(Math::conjc(val)); }
    constexpr inline auto scale(const T k) const { return Gyrovector<T>(Math::mulc(k, val)); }
    constexpr inline auto inv() const { return Gyrovector<T>(Math::invc(val)); }

    constexpr inline auto operator-() const { return Gyrovector<T>(Math::negc(val)); }
    constexpr inline auto operator+() const { return *this; }
};

template<typename T> constexpr auto operator+(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return A.add(B).div(A.conj().mul(B).add(1)); }

template<typename T> constexpr auto operator*(const T k, const Gyrovector<T> & A)
{ auto a = A.abs(); return A.isZero() ? A : A.scale(tanh(k * atanh(a)) / a); }

template<typename T> constexpr Gyrovector<T> gyr(const Gyrovector<T> & A, const Gyrovector<T> & B, const Gyrovector<T> & C) {
    auto P = A.mul(B.conj()).add(1);
    auto Q = A.conj().mul(B).add(1);
    return P.div(Q).mul(C);
}

template<typename T> constexpr Gyrovector<T> Coadd(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto a = A.norm(); auto b = B.norm();
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
    const Gyrovector<double> n₁(0.0, 1.0);
    const auto n₂ = gyr(P₂, -P₁, n₁);
    return std::arg(n₁.val / n₂.val);
}

template<typename T> inline T gyrocos(const Gyrovector<T> & P₁, const Gyrovector<T> & P₂)
{ return (P₁, P₂) / (P₁.abs() * P₂.abs()); }

template<typename T> inline T gyroangle(const Gyrovector<T> & P₁, const Gyrovector<T> & P₂)
{ return acos(gyrocos(P₁, P₂)); }

template<typename T> constexpr T det(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return A.x() * B.y() - B.x() * A.y(); }