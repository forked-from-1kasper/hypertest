#pragma once

#include <ostream>
#include <complex>

#include <Enumerable.hpp>

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

    constexpr Gaussian() : real(0), imag(0) {}
    constexpr Gaussian(T real) : real(real), imag(0) {}
    constexpr Gaussian(T real, T imag) : real(real), imag(imag) {}

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

    // Given N Gaussian integers α and βᵢ (1 ≤ i ≤ N), it multiplies them
    // by ±1/±i so that both of α components become non-negative
    template<std::same_as<Gaussian<T>>... Ts> constexpr void normalize(Ts&... ts) {
        switch (Ord(std::pair(real >= 0, imag >= 0))) {
            /* −1 */ case Ord²(false, false): negate(); (ts.negate(), ...); break;
            /* +i */ case Ord²(true,  false): muli(); (ts.muli(), ...); break;
            /* −i */ case Ord²(false, true):  mulnegi(); (ts.mulnegi(), ...); break;
            /* +1 */ case Ord²(true,  true):  break;
        }

        if (real == 0) { mulnegi(); (ts.mulnegi(), ...); }
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