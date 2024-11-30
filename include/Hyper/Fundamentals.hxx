#pragma once

#include <array>

#include <glm/glm.hpp>
#include <gmpxx.h>

#include <Math/Gyrovector.hxx>

inline constexpr size_t Hword = sizeof(uint8_t);
inline constexpr size_t Word  = sizeof(uint16_t);
inline constexpr size_t Dword = sizeof(uint32_t);
inline constexpr size_t Qword = sizeof(uint64_t);

using Real  = double;
using Real² = std::pair<Real, Real>;
using Real³ = std::tuple<Real, Real, Real>;

using Integer = mpz_class;
using NodeId  = uint16_t;

using Rank  = uint8_t;
using Level = uint8_t;

// Some helpful definitions
template<typename T, int N> using Array² = std::array<std::array<T, N>, N>;

// Various machinery for projections
enum {
    Poincaré    = 1,
    Klein       = 2,
    Gans        = 3,
    Equidistant = 4,
    Lambert     = 5,
};

struct Model {
    int value;

    inline constexpr Model(int value) : value(value) {}
    inline constexpr operator int() const { return value; }

    // Poincaré model → The specified model
    inline constexpr Real² apply(const Real y₁, const Real y₂) const {
        switch (value) {
            case Poincaré: return {y₁, y₂};

            case Klein: {
                auto σ = 1 + y₁ * y₁ + y₂ * y₂;
                return {2 * y₁ / σ, 2 * y₂ / σ};
            }

            case Gans: {
                auto σ = 1 - y₁ * y₁ - y₂ * y₂;
                return {2 * y₁ / σ, 2 * y₂ / σ};
            }

            // http://www.madore.org/~david/programs/#prog_projections
            // https://math.stackexchange.com/questions/1407550/what-hyperbolic-space-really-looks-like
            case Equidistant: {
                /*
                         y = (x / |x|) tanh(τ|x|)
                    →  |y| = (|x| / |x|) tanh(τ|x|) = tanh(τ|x|)
                    ↔ τ|x| = atanh(|y|)
                    ↔  |x| = atanh(|y|) / τ

                    That is, x = (y / |y|) |x| = (y / |y|) atanh(|y|) / τ.
                */
                auto n = Math::hypot(y₁, y₂);

                auto σ = n > 1e-10 ? Math::atanh(n) / (τ * n) : 0;
                return {y₁ * σ, y₂ * σ};
            }

            case Lambert: {
                /*
                                   y  = x / √(1 + |x|²)
                    →            |y|  = |x| / √(1 + |x|²)
                    ↔            |y|² = |x|² / (1 + |x|²)
                    ↔ |y|² (1 + |x|²) = |x|²
                    ↔ |y|² + |x|²|y|² = |x|²
                    ↔ |x|² (1 − |y|²) = |y|²
                    ↔            |x|² = |y|² / (1 − |y|²)
                    ↔        1 + |x|² = (1 − |y|² + |y²|) / (1 − |y|²) = 1 / (1 − |y|²)

                    That is, x = y √(1 + |x|²) = y / √(1 − |y|²).
                */

                auto σ = Math::sqrt(1 - y₁ * y₁ - y₂ * y₂);
                return {y₁ / σ, y₂ / σ};
            }

            default: return {};
        }
    }

    // The specified model → Poincaré model
    inline constexpr Real² unapply(const Real x₁, const Real x₂) const {
        switch (value) {
            case Poincaré: return {x₁, x₂};

            case Klein: {
                auto σ = 1.0 + Math::sqrt(1.0 - x₁ * x₁ - x₂ * x₂);
                return {x₁ / σ, x₂ / σ};
            }

            case Gans: {
                auto σ = 1.0 + Math::sqrt(x₁ * x₁ + x₂ * x₂ + 1.0);
                return {x₁ / σ, x₂ / σ};
            }

            case Equidistant: {
                auto n = Math::hypot(x₁, x₂);

                auto σ = n > 1e-10 ? Math::tanh(τ * n) / n : 0;
                return {x₁ * σ, x₂ * σ};
            }

            case Lambert: {
                auto σ = Math::sqrt(1.0 + x₁ * x₁ + x₂ * x₂);
                return {x₁ / σ, x₂ / σ};
            }

            default: return {};
        }
    }

    inline constexpr auto apply(const Gyrovector<Real> & v) const
    { auto [x₁, x₂] = apply(v.x(), v.y()); return vec2(x₁, x₂); }

    inline constexpr auto unapply(const vec3 w) const
    { auto [x, z] = unapply(w.x, w.z); return vec3(x, w.y, z); }

    inline constexpr auto length(Real value) const
    { auto [x₁, x₂] = apply(value, 0); return Math::hypot(x₁, x₂); }
};

namespace Fundamentals {
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
        has nice representation using integer-valued matrices (see `source/Geometry.cxx`).
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
                    = 2 − √3.
        (See also https://en.wikipedia.org/wiki/List_of_trigonometric_identities, “List of trigonometric identities”.)

        Applying this rule to the other side, we get L:
        L² = (cos(α) + cos(β + γ)) / (cos(α) + cos(β − γ))
           = (cos(τ/4) + cos(θ/2 + θ/2)) / (cos(τ/4) + cos(θ/2 − θ/2))
           = (0 + cos(θ)) / (0 + 1) (because cos(τ/4) = 0 and cos(0) = 1)
           = cos(θ)
           = 1/2.

        Similarly, you can find the length of the entire diagonal:
        D² = (cos(θ) + cos(θ/2 + θ/2)) / (cos(θ) + cos(θ/2 − θ/2))
           = 2cos(θ) / (cos(θ) + 1)
           = 2 / 3,
         D = √(2/3) = √6/3.

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
