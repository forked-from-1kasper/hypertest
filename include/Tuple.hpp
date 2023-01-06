#pragma once

// It is here simply because C++ standard does not require any specific order
// of fields in std::tuple, however we *need* such in VBO.
template<typename...> struct Tuple;

template<> struct Tuple<> {
    constexpr Tuple() {}
};

// Explicit overload is needed to avoid EBCO nuances.
template<typename T> struct Tuple<T> {
    T fin;

    constexpr Tuple() {}
    constexpr Tuple(const T & t) : fin(t) {}
};

template<typename T, typename... Ts> struct Tuple<T, Ts...> {
    T first; Tuple<Ts...> second;

    constexpr Tuple() {}
    constexpr Tuple(const T & t, Ts... ts) : first(t), second(Tuple<Ts...>(ts...)) {}
};

template<class... T> Tuple(T...) -> Tuple<T...>;