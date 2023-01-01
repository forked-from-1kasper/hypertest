#pragma once

#include <array>

#include <glm/glm.hpp>
#include <glm/vec2.hpp>

#include "Gyrovector.hpp"

template<typename T> constexpr T sqr(T x) { return x * x; }
template<typename T> constexpr T sign(T x) { return (x > 0) - (x < 0); }

constexpr double τ = 2 * 3.141592653589793238462643383279502884197169399375105820974944;

using Real    = double;
using Integer = int64_t;
using NodeId  = uint64_t;

using Rank  = uint8_t;
using Level = uint8_t;

// Some helpful definitions
template<typename T, int N> using Array² = std::array<std::array<T, N>, N>;

template<class F, class G> auto compose(F f, G g) {
    return [f, g](auto &&... args) {
        return f(g(std::forward<decltype(args)>(args)...));
    };
}

// Various machinery for projections
enum class Model { Poincaré, Klein, Gans };

namespace Projection {
    constexpr auto model = Model::Gans;

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

        return glm::vec2(x₁, x₂);
    }

    constexpr auto apply(const Gyrovector<Real> & v)
    { return apply(v.x(), v.y()); }
}

namespace Fundamentals {
    constexpr unsigned long textureSize = 16;
    constexpr unsigned long sheetSize = 1024;

    constexpr Level worldTop = 255;

    constexpr Rank exterior = 255;

    constexpr int chunkSize   = 16;
    constexpr int worldHeight = worldTop + 1;

    constexpr auto k = τ / 6;                      // π/3
    constexpr auto D = sqrt(2/(tan(k/2) + 1) - 1); // √(2 − √3)
    constexpr auto L = sqrt(cos(k));               // 1/√2

    const auto gauge = Gyrovector<Real>(D, 0);
    const auto meter = glm::length(Projection::apply(gauge)) / Real(chunkSize);
}
