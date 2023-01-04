#pragma once

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Möbius.hpp>

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