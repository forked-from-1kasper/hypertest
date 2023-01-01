#pragma once

#include <numeric>
#include <complex>
#include <functional>
#include <type_traits>

template<typename T>
struct Gyrovector {
    std::complex<T> val;

    constexpr inline T x() const { return val.real(); }
    constexpr inline T y() const { return val.imag(); }

    constexpr inline T norm() const { return std::norm(val); }
    constexpr inline bool isZero() const { return val.real() == 0.0 && val.imag() == 0.0; }

    constexpr T abs() const {
        if (std::is_constant_evaluated())
            return std::hypot(x(), y());
        else return std::abs(val);
    }

    constexpr Gyrovector() : val(0) {}
    constexpr Gyrovector(const T k) : val(std::complex(k, 0.0)) {}
    constexpr Gyrovector(const T x, const T y) : val(std::complex(x, y)) {}
    constexpr Gyrovector(const std::complex<T> z) : val(z) {}

    constexpr T operator,(const Gyrovector<T> & N) const { return val.real() * N.val.real() + val.imag() * N.val.imag(); }

    constexpr inline auto add(const Gyrovector<T> & N) const { return Gyrovector<T>(val + N.val); }
    constexpr inline auto conj() const { return Gyrovector<T>(std::conj(val)); }
    constexpr inline auto scale(const T k) const { return Gyrovector<T>(k * val); }
    constexpr inline auto mult(const Gyrovector<T> & N) const { return Gyrovector<T>(val * N.val); }
    constexpr inline auto inv() const { return Gyrovector<T>(1.0 / val); }
    constexpr inline auto div(const Gyrovector<T> & N) const { return Gyrovector<T>(val / N.val); }

    constexpr inline auto operator-() const { return Gyrovector<T>(-val); }
    constexpr inline auto operator+() const { return *this; }
};

template<typename T> constexpr auto operator+(const Gyrovector<T> & A, const Gyrovector<T> & B)
{ return A.add(B).div(A.conj().mult(B).add(1)); }

template<typename T> constexpr auto operator*(const T k, const Gyrovector<T> & A)
{ auto a = A.abs(); return A.isZero() ? A : A.scale(tanh(k * atanh(a)) / a); }

template<typename T> constexpr Gyrovector<T> gyr(const Gyrovector<T> & A, const Gyrovector<T> & B, const Gyrovector<T> & C) {
    auto P = A.mult(B.conj()).add(1);
    auto Q = A.conj().mult(B).add(1);
    return P.div(Q).mult(C);
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

template<typename T>
struct Möbius {
    constexpr static auto zero = Gyrovector<T>(0, 0);

    std::complex<T> a, b, c, d;

    constexpr std::complex<T> det() const { return a * d - b * c; }

    constexpr inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    constexpr inline Möbius<T> normalize() const { return div(sqrt(det())); }

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