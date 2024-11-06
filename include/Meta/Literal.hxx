#pragma once

#include <algorithm>

template<size_t N> struct Literal {
    char unquote[N];

    constexpr Literal(const char (&buf)[N]) { std::copy_n(buf, N, unquote); }

    static inline constexpr size_t size = N;
};
