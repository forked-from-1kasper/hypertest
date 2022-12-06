#include "Gyrovector.hpp"

#pragma once

constexpr double τ = 2 * 3.141592653589793238462643383279502884197169399375105820974944;

using Real    = double;
using Integer = int64_t;
using NodeId  = uint64_t;

enum class Model { Poincaré, Klein, Gans };
constexpr auto model = Model::Gans;

namespace Projection {
    constexpr auto apply(Real y₁, Real y₂) {
        Real x₁, x₂;

        switch (model) {
            case Model::Poincaré: x₁ = y₁; x₂ = y₂; break;

            case Model::Klein: {
                auto σ = 1 + y₁ * y₁ + y₂ * y₂;
                x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ; break;
            };

            case Model::Gans: {
                auto σ = 1 - y₁ * y₁ - y₂ * y₂;
                x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ; break;
            }
        }

        return Vector2<Real>(x₁, x₂);
    }

    constexpr auto apply(const Gyrovector<Real> & v)
    { return apply(v.x(), v.y()); }
}

namespace Fundamentals {
    constexpr int chunkSize   = 16;
    constexpr int worldHeight = 256;

    constexpr auto k = τ / 6;                      // π/3
    constexpr auto D = sqrt(2/(tan(k/2) + 1) - 1); // √(2 − √3)
    constexpr auto L = sqrt(cos(k));               // 1/√2

    constexpr auto gauge = Gyrovector<Real>(D, +0);
    constexpr auto meter = Projection::apply(gauge).abs() / Real(chunkSize);
}

template<typename T, int N> using Array² = std::array<std::array<T, N>, N>;

template<typename T> constexpr T sign(T x) { return (x > 0) - (x < 0); }

// Generation of chunk’s grid
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

constexpr auto yieldGrid(int i, int j) {
    using namespace Fundamentals;

    auto x = Real(2 * i - chunkSize) / Real(chunkSize);
    auto y = Real(2 * j - chunkSize) / Real(chunkSize);
    return Ψ(x, y);
}

constexpr auto initGrid() {
    using namespace Fundamentals;

    Array²<Gyrovector<Real>, chunkSize + 1> retval;

    for (int i = 0; i <= chunkSize; i++)
        for (int j = 0; j <= chunkSize; j++)
            retval[i][j] = yieldGrid(i, j);

    return retval;
}

constexpr auto grid = initGrid();

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

template<class F, class G> auto compose(F f, G g) {
    return [f, g](auto &&... args) {
        return f(g(std::forward<decltype(args)>(args)...));
    };
}