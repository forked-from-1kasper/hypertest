#pragma once

#include <algorithm>

template<size_t N>
struct Literal {
    char unquote[N];
    constexpr Literal(const char (&lit)[N]) { std::copy_n(lit, N, unquote); }
};