#pragma once

#include <type_traits>
#include <concepts>

template<typename...> struct List;

template<typename T, typename... Ts> struct List<T, Ts...>
{ using head = T; using tail = List<Ts...>; };

template<typename U> concept Empty = std::same_as<U, List<>>;

template<typename U> concept Inhabited = requires()
{ typename U::head; typename U::tail; };

template<Inhabited U> using Head = typename U::head;
template<Inhabited U> using Tail = typename U::tail;

template<typename, typename> struct ConsM;
template<typename T, typename Ts>
using Cons = typename ConsM<T, Ts>::value;

template<typename T, typename... Ts>
struct ConsM<T, List<Ts...>> { using value = List<T, Ts...>; };

template<template<typename...> typename, typename> struct ApplyM;
template<template<typename...> typename M, typename T>
using Apply = typename ApplyM<M, T>::value;

template<template<typename...> typename M, typename... Ts>
struct ApplyM<M, List<Ts...>> { using value = M<Ts...>; };

template<template<typename> typename, typename> struct MapM;
template<template<typename> typename φ, typename Ts>
using Map = typename MapM<φ, Ts>::value;

template<template<typename> typename φ>
struct MapM<φ, List<>> { using value = List<>; };

template<template<typename> typename φ, typename T, typename... Ts>
struct MapM<φ, List<T, Ts...>> { using value = Cons<φ<T>, Map<φ, List<Ts...>>>; };

template<typename> struct LengthM;

template<typename... Ts> struct LengthM<List<Ts...>>
{ constexpr static size_t value = sizeof...(Ts); };

template<typename T> constexpr static size_t Length = LengthM<T>::value;

template<int k, typename T> struct EnumerateItem
{ static constexpr int index = k; using typeval = T; };

template<int, typename...> struct EnumerateM;
template<typename... Ts> using Enumerate = EnumerateM<0, Ts...>::value;

template<int k> struct EnumerateM<k>
{ using value = List<>; };

template<int k, typename T, typename... Ts> struct EnumerateM<k, T, Ts...>
{ using value = Cons<EnumerateItem<k, T>, typename EnumerateM<k + 1, Ts...>::value>; };
