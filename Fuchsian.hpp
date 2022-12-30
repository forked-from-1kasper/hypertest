#pragma once

#include <ostream>
#include <complex>

#include "Gyrovector.hpp"
#include "Enumerable.hpp"

template<typename T> concept EuclideanDomain =
requires(T a, T b) {
    {     0     } -> std::convertible_to<T>;
    {     1     } -> std::convertible_to<T>;
    {    -a     } -> std::convertible_to<T>;
    {   a + b   } -> std::convertible_to<T>;
    {   a - b   } -> std::convertible_to<T>;
    {   a * b   } -> std::convertible_to<T>;
    {   a / b   } -> std::convertible_to<T>;
    {   a % b   } -> std::convertible_to<T>;
    {   a << b  } -> std::convertible_to<T>;
    {   a >> b  } -> std::convertible_to<T>;
};

template<EuclideanDomain T>
struct Gaussian {
    T real, imag;

    constexpr inline auto operator-() const { return Gaussian(-real, -imag); }
    constexpr inline auto operator+() const { return *this; }

    constexpr auto operator+(const Gaussian<T> & w) const
    { return Gaussian<T>(real + w.real, imag + w.imag); }

    constexpr auto operator+=(const Gaussian<T> & w)
    { real += w.real; imag += w.imag; return *this; }

    constexpr auto operator-(const Gaussian<T> & w) const
    { return Gaussian<T>(real - w.real, imag - w.imag); }

    constexpr auto operator-=(const Gaussian<T> & w)
    { real -= w.real; imag -= w.imag; return *this; }

    constexpr auto operator*(const Gaussian<T> & w) const
    { return Gaussian<T>(real * w.real - imag * w.imag, real * w.imag + imag * w.real); }

    constexpr auto operator*=(const Gaussian<T> & w) {
        auto α(real), β(w.real);
        real = α * β - imag * w.imag;
        imag = α * w.imag + imag * β;
        return *this;
    }

    constexpr auto operator/(const T & k) const { return Gaussian<T>(real / k, imag / k); }
    constexpr auto operator/=(const T & k) { real /= k; imag /= k; return *this; }

    constexpr auto norm() const { return real * real + imag * imag; }

    constexpr auto operator/(const Gaussian<T> & w) const {
        auto N = w.norm();
        auto x = real * w.real + imag * w.imag;
        auto y = imag * w.real - real * w.imag;
        return Gaussian<T>(x / N, y / N);
    }

    constexpr auto isZero() const { return real == 0 && imag == 0; }
    constexpr auto isUnit() const { return (abs(real) == 1 && imag == 0)
                                        || (real == 0 && abs(imag) == 1); }

    constexpr void negate()  { real = -real; imag = -imag; }
    constexpr void twice()   { real <<= 1; imag <<= 1; }
    constexpr void half()    { real >>= 1; imag >>= 1; }
    constexpr void mulω()    { real -= imag; imag <<= 1; imag += real; }
    constexpr void mulnegi() { std::swap(real, imag); imag = -imag; }
    constexpr void muli()    { std::swap(real, imag); real = -real; }
    constexpr void divω()    { real += imag; imag <<= 1; imag -= real; half(); }

    constexpr auto kind() const { return std::pair(bool(real % 2), bool(imag % 2)); }

    // https://www.researchgate.net/publication/269005874_A_Paper-and-Pencil_gcd_Algorithm_for_Gaussian_Integers
    // https://www.researchgate.net/publication/325472716_PERFORMANCE_OF_A_GCD_ALGO-RITHM_FOR_GAUSSIAN_INTEGERS
    constexpr static auto hcf(Gaussian<T> α, Gaussian<T> β) {
        Gaussian<T> δ(1, 0);

        for (;;) {
            if (α == β || α == -β) return α * δ;
            if (α.isUnit() || β.isUnit()) return δ;
            if (α.isZero()) return β * δ;
            if (β.isZero()) return α * δ;

            switch (Ord(std::pair(α.kind(), β.kind()))) {
                case Digit(0, 0, 0, 0): α.half(); β.half(); δ.twice(); break;
                case Digit(0, 0, 1, 1): α.half(); β.divω(); δ.mulω();  break;
                case Digit(1, 1, 0, 0): α.divω(); β.half(); δ.mulω();  break;
                case Digit(1, 1, 1, 1): α.divω(); β.divω(); δ.mulω();  break;

                case Digit(0, 1, 0, 0): case Digit(1, 0, 0, 0): β.half(); break;
                case Digit(0, 0, 0, 1): case Digit(0, 0, 1, 0): α.half(); break;
                case Digit(1, 0, 0, 1): case Digit(0, 1, 1, 0): β.muli(); break;
                case Digit(1, 1, 0, 1): case Digit(1, 1, 1, 0): α.divω(); break;
                case Digit(0, 1, 1, 1): case Digit(1, 0, 1, 1): β.divω(); break;

                case Digit(1, 0, 1, 0): case Digit(0, 1, 0, 1):
                α += β; β.twice(); β -= α; α.half(); β.half(); break;
            }
        }
    }

    // Given two Gaussian integers α and β, it multiplies them by ±1/±i
    // so that both of α components become positive
    constexpr auto normalizeRational(Gaussian<T> & δ) {
        switch (Ord(std::pair(real >= 0, imag >= 0))) {
            /* −1 */ case Ord²(false, false): negate(); δ.negate(); break;
            /* +i */ case Ord²(true,  false): muli(); δ.muli(); break;
            /* −i */ case Ord²(false, true):  mulnegi(); δ.mulnegi(); break;
            /* +1 */ case Ord²(true,  true):  break;
        }
    }

    constexpr auto operator==(const Gaussian<T> & w) const
    { return real == w.real && imag == w.imag; }

    constexpr auto operator!=(const Gaussian<T> & w) const
    { return real != w.real || imag != w.imag; }

    template<typename U> constexpr auto field() const { return std::complex<U>(real, imag); }
    template<EuclideanDomain U> auto transform() const { return Gaussian<U>(real, imag); }

    friend std::ostream & operator<< (std::ostream & stream, const Gaussian<T> & z)
    { return stream << z.real << " + " << z.imag << "i"; }
};

template<EuclideanDomain T>
using Gaussian² = std::pair<Gaussian<T>, Gaussian<T>>;

template<EuclideanDomain T>
struct Fuchsian {
    static constexpr auto s = sqrt(6);
    Gaussian<T> a, b, c, d;

    constexpr Gaussian<T> det() const { return a * d - b * c; }

    void simpl() {
        auto σ(a);

        σ = Gaussian<T>::hcf(b, σ);
        σ = Gaussian<T>::hcf(c, σ);
        σ = Gaussian<T>::hcf(d, σ);

        a = a / σ; b = b / σ; c = c / σ; d = d / σ;
    }

    template<typename U> constexpr inline Möbius<U> field() const {
        return { a.template field<U>(),     b.template field<U>() / s,
                 c.template field<U>() * s, d.template field<U>() };
    }

    constexpr inline auto inverse() const { return Fuchsian<T>(d, -b, -c, a); }

    constexpr inline auto origin() const {
        auto σ = Gaussian<T>::hcf(b, d), α = b / σ, β = d / σ;
        α.normalizeRational(β); return std::pair(α, β);
    }

    constexpr auto operator==(const Fuchsian<T> & G) const
    { return a == G.a && b == G.b && c == G.b && d == G.d; }

    friend std::ostream & operator<< (std::ostream & stream, const Fuchsian<T> & G)
    { return stream << "(" << G.a << ", " << G.b << ", " << G.c << ", " << G.d << ")"; }
};

template<typename T> constexpr Fuchsian<T> operator*(const Fuchsian<T> & A, const Fuchsian<T> & B) {
    return {
        A.a * B.a + A.b * B.c,
        A.a * B.b + A.b * B.d,
        A.c * B.a + A.d * B.c,
        A.c * B.b + A.d * B.d
    };
}

template<typename T> void operator*=(Fuchsian<T> & A, const Fuchsian<T> & B) { A = A * B; A.simpl(); }