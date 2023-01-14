#pragma once

#include <Hyper/Moebius.hpp>
#include <Hyper/Gaussian.hpp>

template<EuclideanDomain T>
struct Fuchsian {
    Gaussian<T> a, b, c, d;

    constexpr Fuchsian() {}
    constexpr Fuchsian(auto a, auto b, auto c, auto d) : a(a), b(b), c(c), d(d) {}

    constexpr Gaussian<T> det() const { return a * d - b * c; }

    constexpr void simpl() {
        auto σ(a);

        σ = Gaussian<T>::hcf(b, σ);
        σ = Gaussian<T>::hcf(c, σ);
        σ = Gaussian<T>::hcf(d, σ);

        a = a / σ; b = b / σ; c = c / σ; d = d / σ;
    }

    template<typename U> constexpr inline Möbius<U> field() const {
        // See `source/Geometry.cpp` for “s” meaning.
        #ifdef __clang__
            constexpr U s = 2.449489742783178;
        #else
            constexpr U s = sqrt(6.0);
        #endif

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