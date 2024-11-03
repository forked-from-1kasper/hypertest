#pragma once

#include <array>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/glm.hpp>
#include <gmpxx.h>

#include <Hyper/Gyrovector.hpp>

using Real    = double;
using Integer = mpz_class;
using NodeId  = uint16_t;

using Rank  = uint8_t;
using Level = uint8_t;

// Some helpful definitions
template<typename T, int N> using Array² = std::array<std::array<T, N>, N>;

// Various machinery for projections
enum class Model {
    Poincaré = 1,
    Klein    = 2,
    Gans     = 3
};

namespace Projection {
    // Poincaré model → The specified model
    constexpr auto apply(Model model, const Real y₁, const Real y₂) {
        Real x₁, x₂;

        switch (model) {
            case Model::Poincaré: x₁ = y₁; x₂ = y₂; break;

            case Model::Klein: {
                auto σ = 1 + y₁ * y₁ + y₂ * y₂;
                x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ;
                break;
            }

            case Model::Gans: {
                auto σ = 1 - y₁ * y₁ - y₂ * y₂;
                x₁ = 2 * y₁ / σ; x₂ = 2 * y₂ / σ;
                break;
            }
        }

        return std::pair(x₁, x₂);
    }

    // The specified model → Poincaré model
    inline constexpr auto unapply(Model model, const Real x₁, const Real x₂) {
        float y₁, y₂;

        switch (model) {
            case Model::Poincaré: y₁ = x₁; y₂ = x₂; break;

            case Model::Klein: {
                auto σ = 1.0 + Math::sqrt(1.0 - x₁ * x₁ - x₂ * x₂);
                y₁ = x₁ / σ; y₂ = x₂ / σ;
                break;
            }

            case Model::Gans: {
                auto σ = 1.0 + Math::sqrt(x₁ * x₁ + x₂ * x₂ + 1.0);
                y₁ = x₁ / σ; y₂ = x₂ / σ;
                break;
            }
        }

        return std::pair(y₁, y₂);
    }

    inline constexpr auto apply(Model model, const Gyrovector<Real> & v)
    { auto [x₁, x₂] = apply(model, v.x(), v.y()); return glm::vec2(x₁, x₂); }

    inline constexpr auto unapply(Model model, const glm::vec3 w)
    { auto [x, z] = unapply(model, w.x, w.z); return glm::vec3(x, w.y, z); }

    inline const auto length(Model model, Real value)
    { return glm::length(Projection::apply(model, Gyrovector<Real>(value, 0))); }
}

namespace Fundamentals {
    constexpr unsigned long textureSize = 16;
    constexpr unsigned long sheetSize = 1024;

    constexpr Level worldTop = 255;

    constexpr Rank exterior = 255;

    constexpr int chunkSize   = 16;
    constexpr int worldHeight = worldTop + 1;

    // https://www.researchgate.net/publication/299161235_THE_HYPERBOLIC_SSQUARE_AND_MOBIUS_TRANSFORMATIONS
    // https://link.springer.com/book/10.1007/978-3-031-02396-5, “A Gyrovector Space Approach to Hyperbolic Geometry”
    // https://www.amazon.com/Analytic-Hyperbolic-Geometry-Einsteins-Relativity/dp/9811244103, “Analytic Hyperbolic Geometry and Albert Einstein’s Special Theory of Relativity”

    constexpr Real k  = τ / 6;                                  // π/3, angle of chunk’s square
    constexpr Real D½ = Math::sqrt(2/(Math::tan(k/2) + 1) - 1); // √(2 − √3), gyrodiagonal “half gyrolength” of chunk’s gyrosquare
    constexpr Real L  = Math::sqrt(Math::cos(k));               // 1/√2, chunk’s side (gyro)length

    /*
        “k = τ/6” is used because corresponding tesselation (https://en.wikipedia.org/wiki/Order-6_square_tiling)
        has nice representation using integer-valued matrices (see `source/Geometry.cpp`).
        (See also https://proceedings.neurips.cc/paper/2019/file/82c2559140b95ccda9c6ca4a8b981f1e-Paper.pdf,
         “Numerically Accurate Hyperbolic Embeddings Using Tiling-Based Models”.)

        D½ can be calculated from hyperbolic AAA to SSS conversion law.
        We have a triangle with two half diagonals and one square’s side as sides, its angles — α = τ/4, β = θ/2 and γ = θ/2 (θ = τ/6).
        So then D½² = (cos(γ) + cos(α + β)) / (cos(γ) + cos(α - β))
                    = (cos(θ/2) + cos(τ/4 + θ/2)) / (cos(θ/2) + cos(τ/4 − θ/2))
                    = (cos(θ/2) − sin(θ/2)) / (cos(θ/2) + sin(θ/2)) (because cos(τ/4 + x) = −sin(x) and cos(τ/4 − x) = sin(x))
                    = (2cos(θ/2) − cos(θ/2) − sin(θ/2)) / (cos(θ/2) + sin(θ/2))
                    = 2cos(θ/2) / (cos(θ/2) + sin(θ/2)) − 1
                    = 2/(1 + sin(θ/2)/cos(θ/2)) − 1
                    = 2/(1 + tan(θ/2)) − 1
                    = 2 − √3
        (See also https://en.wikipedia.org/wiki/List_of_trigonometric_identities, “List of trigonometric identities”.)

        Applying this rule to the other side, we get L:
        L² = (cos(α) + cos(β + γ)) / (cos(α) + cos(β − γ))
           = (cos(τ/4) + cos(θ/2 + θ/2)) / (cos(τ/4) + cos(θ/2 − θ/2))
           = (0 + cos(θ)) / (0 + 1) (because cos(τ/4) = 0 and cos(0) = 1)
           = cos(θ)
           = 1/2

        Similarly, you can find the length of the entire diagonal:
        D² = (cos(θ) + cos(θ/2 + θ/2)) / (cos(θ) + cos(θ/2 − θ/2))
           = 2cos(θ) / (cos(θ) + 1)
           = 2 / 3,
         D = √(2/3) = √6/3

        Note that 2 × D½ ≠ D. However, gyrodistance in Poincaré disk model is given by the formula:
            d(u, v) = |−u ⨁ v| = |−u + v|/|1 − vu*|
        So d(−D½, D½) = 2D½/|1 − D½ × (−D½)*|       = 2D½/|1 + D½²|
                      = 2√(2 − √3) / (3 − √3)       = 2√[(2 − √3)/(3 − √3)²]
                      = 2√[(2 − √3)/(9 − 6√3 + 3)]  = 2√[(2 − √3)/(12 − 6√3)]
                      = 2√(1/6)                     = √(4/6)
                      = √(2/3)                      = D,
        as expected.
    */
}
