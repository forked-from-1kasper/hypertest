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