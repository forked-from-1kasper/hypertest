#pragma once

#include <ostream>
#include <complex>

#include <Hyper/EuclideanDomain.hpp>
#include <Meta/Enumerable.hpp>

template<EuclideanDomain T> struct Gaussian {
    T real, imag;

    constexpr Gaussian() : real(Math::zero<T>), imag(Math::zero<T>) {}
    constexpr Gaussian(const T & real) : real(real), imag(Math::zero<T>) {}
    constexpr Gaussian(const T & real, const T & imag) : real(real), imag(imag) {}

    constexpr inline auto operator-() const { return Gaussian(-real, -imag); }
    constexpr inline auto & operator+() const { return *this; }

    constexpr auto operator+(const Gaussian<T> & w) const
    { return Gaussian<T>(real + w.real, imag + w.imag); }

    constexpr void operator+=(const Gaussian<T> & w)
    { real += w.real; imag += w.imag; }

    constexpr auto operator-(const Gaussian<T> & w) const
    { return Gaussian<T>(real - w.real, imag - w.imag); }

    constexpr void operator-=(const Gaussian<T> & w)
    { real -= w.real; imag -= w.imag; }

    constexpr auto operator*(const Gaussian<T> & w) const
    { return Gaussian<T>(real * w.real - imag * w.imag, real * w.imag + imag * w.real); }

    constexpr void operator*=(const Gaussian<T> & w) {
        auto α(real), β(w.real);
        real = α * β - imag * w.imag;
        imag = α * w.imag + imag * β;
    }

    constexpr inline T norm() const { return real * real + imag * imag; }

    constexpr auto operator/(const T & k) const { return Gaussian<T>(real / k, imag / k); }
    constexpr void operator/=(const T & k) { real /= k; imag /= k; }

    constexpr auto operator/(const Gaussian<T> & w) const {
        auto N = w.norm();
        T x(real * w.real + imag * w.imag);
        T y(imag * w.real - real * w.imag);
        return Gaussian<T>(x / N, y / N);
    }

    constexpr inline void divexact(const T & k) {
        Math::divexact<T>(real, real, k);
        Math::divexact<T>(imag, imag, k);
    }

    constexpr void divexact(const Gaussian<T> & w) {
        auto N = w.norm();

        T x(real * w.real + imag * w.imag);
        T y(imag * w.real - real * w.imag);

        Math::divexact<T>(real, x, N);
        Math::divexact<T>(imag, y, N);
    }

    constexpr auto isZero() const { return Math::isZero(real) && Math::isZero(imag); }
    constexpr auto isUnit() const { return (Math::isUnit(real) && Math::isZero(imag))
                                        || (Math::isZero(real) && Math::isUnit(imag)); }

    constexpr void negate()  { real = -real; imag = -imag; }
    constexpr void twice()   { Math::twice(real); Math::twice(imag); }
    constexpr void half()    { Math::half(real); Math::half(imag); }
    constexpr void mulω()    { real -= imag; Math::twice(imag); imag += real; }
    constexpr void mulnegi() { std::swap(real, imag); imag = -imag; }
    constexpr void muli()    { std::swap(real, imag); real = -real; }
    constexpr void divω()    { real += imag; Math::twice(imag); imag -= real; half(); }

    constexpr auto kind() const { return std::pair(Math::odd(real), Math::odd(imag)); }

    // https://www.researchgate.net/publication/269005874_A_Paper-and-Pencil_gcd_Algorithm_for_Gaussian_Integers
    // https://www.researchgate.net/publication/325472716_PERFORMANCE_OF_A_GCD_ALGO-RITHM_FOR_GAUSSIAN_INTEGERS
    constexpr static auto hcf(Gaussian<T> α, Gaussian<T> β) {
        Gaussian<T> δ(Math::one<T>);

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

        if (Math::isZero<T>(real)) { mulnegi(); (ts.mulnegi(), ...); }
    }

    constexpr auto operator==(const Gaussian<T> & w) const
    { return Math::equal<T>(real, w.real) && Math::equal<T>(imag, w.imag); }

    constexpr auto operator!=(const Gaussian<T> & w) const
    { return Math::differ<T>(real, w.real) || Math::differ<T>(imag, w.imag); }

    template<EuclideanDomain U> auto transform() const { return Gaussian<U>(real, imag); }

    template<typename U> constexpr auto field() const {
        return std::complex<U>(Math::field<T, U>(real), Math::field<T, U>(imag));
    }

    friend std::ostream & operator<< (std::ostream & stream, const Gaussian<T> & z)
    { return stream << z.real << " + " << z.imag << "i"; }
};

template<EuclideanDomain T>
using Gaussian² = std::pair<Gaussian<T>, Gaussian<T>>;