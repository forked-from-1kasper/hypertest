#pragma once

#include "Fundamentals.hpp"

// Generation of chunk’s grid
namespace Grid {
    constexpr auto Φ(Real x, Real y) {
        using namespace Fundamentals;

        if (x == 0 && y == 0) return Gyrovector<Real>(0, 0);

        auto L = fabs(x) + fabs(y);

        Gyrovector<Real> u(sign(x) * L, 0), v(0, sign(y) * L);
        return u + fabs(y / L) * (-u + v);
    }

    constexpr auto Ψ(Real x, Real y) {
        using namespace Fundamentals;

        auto u = (x + y) / 2;
        auto v = (x - y) / 2;

        return Φ(u * D, v * D);
    }

    constexpr auto yield(int i, int j) {
        using namespace Fundamentals;

        auto x = Real(2 * i - chunkSize) / Real(chunkSize);
        auto y = Real(2 * j - chunkSize) / Real(chunkSize);
        return Ψ(x, y);
    }

    constexpr auto init() {
        using namespace Fundamentals;

        Array²<Gyrovector<Real>, chunkSize + 1> retval;

        for (int i = 0; i <= chunkSize; i++)
            for (int j = 0; j <= chunkSize; j++)
                retval[i][j] = yield(i, j);

        return retval;
    }

    constexpr auto corners = init();
}

// Chunk’s neighbours in tesselation
namespace Array {
    template<std::size_t N, typename T, typename U>
    constexpr auto inverse(const std::array<Fuchsian<T>, N> & xs) {
        std::array<Möbius<U>, N> retval;

        for (size_t i = 0; i < N; i++)
            retval[i] = xs[i].inverse().template field<U>();

        return retval;
    }
}

namespace Tesselation {
    constexpr Fuchsian<Integer> I {{+1, +0}, {+0, +0}, {+0, +0}, {+1, +0}};
    constexpr Fuchsian<Integer> U {{+6, +0}, {+6, +6}, {+1, -1}, {+6, +0}};
    constexpr Fuchsian<Integer> L {{+6, +0}, {+6, -6}, {+1, +1}, {+6, +0}};
    constexpr Fuchsian<Integer> D {{+6, +0}, {-6, -6}, {-1, +1}, {+6, +0}};
    constexpr Fuchsian<Integer> R {{+6, +0}, {-6, +6}, {-1, -1}, {+6, +0}};

    constexpr std::array neighbours {
        U, L, D, R,
        // corners
        U * L, U * L * D, U * L * D * R,
        U * R, U * R * D, U * R * D * L,
        D * L, D * L * U, D * L * U * R,
        D * R, D * R * U, D * R * U * L
    };

    constexpr auto neighbours⁻¹ = Array::inverse<neighbours.size(), Integer, Real>(neighbours);
}