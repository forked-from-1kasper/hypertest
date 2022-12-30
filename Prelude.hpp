#pragma once

template<typename T> constexpr T sqr(T x) { return x * x; }
template<typename T> constexpr T sign(T x) { return (x > 0) - (x < 0); }