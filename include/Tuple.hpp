#pragma once

template<typename...> struct Tuple;

template<> struct Tuple<> {
    constexpr Tuple() {}
};

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