#pragma once

#include <cmath>
#include <complex>
#include <numeric>

template<typename T> inline constexpr T Ln2   = 0.693147180559945309417232121458176568075500134360255254120680;
template<typename T> inline constexpr T Sqrt2 = 1.414213562373095048801688724209698078569671875376948073176680;
template<typename T> inline constexpr T Pi    = 3.141592653589793238462643383279502884197169399375105820974944;
template<typename T> inline constexpr T Tau   = 2.0 * Pi<T>;

inline constexpr auto ln2 = Ln2<double>, sqrt2 = Sqrt2<double>, π = Pi<double>, τ = Tau<double>;

namespace Math {
    template<typename T> inline constexpr T sqr(T x)  { return x * x; }
    template<typename T> inline constexpr T abs(T x)  { return x > 0 ? x : -x; }
    template<typename T> inline constexpr T sign(T x) { return (x > 0) - (x < 0); }

    template<typename...> struct EqualM;

    template<> struct EqualM<> { static inline bool apply() { return true; } };

    template<typename T> struct EqualM<T> { static inline bool apply(T) { return true; } };

    template<typename T₁, typename T₂, typename... Ts> struct EqualM<T₁, T₂, Ts...> {
        static inline bool apply(T₁ t₁, T₂ t₂, Ts... ts)
        { return (t₁ == t₂) && EqualM<T₂, Ts...>::apply(t₂, ts...); }
    };

    template<typename... Ts> inline bool equal(Ts... ts) { return EqualM<Ts...>::apply(ts...); }
    template<typename... Ts> inline bool samesign(Ts... ts) { return equal((ts < 0)...); }

    template<typename T> constexpr T NewtonRaphson(T x, T curr, T prev)
    { return curr == prev ? curr : NewtonRaphson<T>(x, 0.5 * (curr + x / curr), curr); }

    // https://en.cppreference.com/w/cpp/numeric/math/sqrt (constexpr since C++26)
    template<typename T> inline constexpr T sqrt(T x) {
        if (std::is_constant_evaluated())
            // https://en.wikipedia.org/wiki/Newton%27s_method
            return 0 <= x && x < std::numeric_limits<double>::infinity()
                 ? NewtonRaphson<T>(x, x, 0) : std::numeric_limits<double>::quiet_NaN();
        else return std::sqrt(x);
    }

    // https://en.cppreference.com/w/cpp/numeric/math/hypot (constexpr since C++26)
    template<typename T> inline constexpr T hypot(T x, T y) {
        if (std::is_constant_evaluated())
            return Math::sqrt(x * x + y * y);
        else return std::hypot(x, y);
    }

    template<typename T> inline constexpr T modulo(T x, T y) {
        if constexpr(std::is_floating_point<T>()) {
            if (std::is_constant_evaluated())
                return x - static_cast<long long>(x / y) * y;
            else
                return std::fmod(x, y);
        } else return x % y;
    }

    // Just an arbitrary small number, not the actual precision.
    template<typename T> inline constexpr T epsilon = 1e-13;

    template<typename T> inline constexpr T sin(T φ) {
        if (std::is_constant_evaluated()) {
            T x = modulo<T>(φ, Tau<T>);

            x = x > 1.5 * Pi<T> ? x - Tau<T> : x > 0.5 * Pi<T> ? Pi<T> - x : x;

            const T x² = x * x; T y = x;

            // Maclaurin series: sin(x) = Σₙ (−1)ⁿx²ⁿ⁺¹/(2n + 1)!
            for (size_t i = 1; epsilon<T> <= Math::abs(x); i++)
            { x *= -x²; x /= 2 * i; x /= 2 * i + 1; y += x; }

            return y;
        } else return std::sin(φ);
    }

    template<typename T> inline constexpr T cos(T x) {
        if (std::is_constant_evaluated())
            return Math::sin<T>(Tau<T> / 4.0 - x);
        else
            return std::cos(x);
    }

    template<typename T> inline constexpr T tan(T x) {
        if (std::is_constant_evaluated())
            return Math::sin<T>(x) / Math::cos<T>(x);
        else
            return std::tan(x);
    }

    template<typename T> inline constexpr T cot(T x) {
        if (std::is_constant_evaluated())
            return Math::cos<T>(x) / Math::sin<T>(x);
        else
            return 1.0 / std::tan(x);
    }

    template<typename T> inline constexpr T pow2(T n)
    { return n > 0 ? 2 << (n - 1) : n < 0 ? 2 << (-n - 1) : 1; }

    template<typename T> inline constexpr T exp(T x) {
        if (std::is_constant_evaluated()) {
            /* exp(x) = exp(r + q × ln(2))
                      = exp(ln(2^q)) × exp(r)
                      = 2^q × exp(r)
            */
            T q = static_cast<long long>(x / Ln2<T>), r = x - q * Ln2<T>;

            T x = 1, y = 0;

            // Maclaurin series: exp(x) = Σₙ xⁿ/n!
            for (size_t i = 1; epsilon<T> <= Math::abs(x); i++)
            { y += x; x *= r; x /= i; }

            return y * pow2<long long>(q);
        } else return std::exp(x);
    }

    template<typename T> inline constexpr T atanh(T x) {
        if (std::is_constant_evaluated()) {
            const T x² = x * x; T y = x;

            // Maclaurin series: atanh(x) = Σₙ x²ⁿ⁺¹/(2n + 1)
            for (size_t i = 1; epsilon<T> <= Math::abs(x); i++)
            { x *= x²; y += x / (2 * i + 1); }

            return y;
        } else return std::atanh(x);
    }

    template<typename T> inline constexpr T sinh(T x) {
        if (std::is_constant_evaluated())
            return 0.5 * (Math::exp(x) - Math::exp(-x));
        else return std::sinh(x);
    }

    template<typename T> inline constexpr T cosh(T x) {
        if (std::is_constant_evaluated()) {
            return 0.5 * (Math::exp(x) + Math::exp(-x));
        } else return std::cosh(x);
    }

    template<typename T> inline constexpr T tanh(T x) {
        if (std::is_constant_evaluated()) {
            T a = Math::exp(x), b = Math::exp(-x);
            return (a - b) / (a + b);
        } else return std::tanh(x);
    }

    template<typename T> using ℂ = std::complex<T>;

    // https://github.com/llvm/llvm-project/issues/55370
    template<typename T> inline constexpr ℂ<T> addc(ℂ<T> v, ℂ<T> w) {
        if (std::is_constant_evaluated())
            return {v.real() + w.real(), v.imag() + w.imag()};
        else return v + w;
    }

    template<typename T> inline constexpr ℂ<T> subc(ℂ<T> v, ℂ<T> w) {
        if (std::is_constant_evaluated())
            return {v.real() - w.real(), v.imag() - w.imag()};
        else return v - w;
    }

    template<typename T> inline constexpr ℂ<T> mulc(ℂ<T> v, ℂ<T> w) {
        if (std::is_constant_evaluated())
            return {
                v.real() * w.real() - v.imag() * w.imag(),
                v.real() * w.imag() + v.imag() * w.real()
            };
        else return v * w;
    }

    template<typename T> inline constexpr T absc(ℂ<T> v) {
        if (std::is_constant_evaluated())
            return Math::hypot<T>(v.real(), v.imag());
        else return std::abs(v);
    }

    template<typename T> inline constexpr T normc(ℂ<T> v) {
        if (std::is_constant_evaluated())
            return v.real() * v.real() + v.imag() * v.imag();
        else return std::norm(v);
    }

    template<typename T> inline constexpr ℂ<T> divc(ℂ<T> v, ℂ<T> w) {
        if (std::is_constant_evaluated()) {
            auto N = Math::normc<T>(w);

            T x(v.real() * w.real() + v.imag() * w.imag());
            T y(v.imag() * w.real() - v.real() * w.imag());

            return {x / N, y / N};
        } else return v / w;
    }
}
