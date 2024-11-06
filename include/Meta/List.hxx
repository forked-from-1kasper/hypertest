#pragma once

#include <type_traits>
#include <concepts>

template<typename...> struct List;

template<typename T, typename... Ts> struct List<T, Ts...>
{ using head = T; using tail = List<Ts...>; };

template<typename, typename> struct ConsM;
template<typename T, typename Ts>
using Cons = typename ConsM<T, Ts>::value;

template<typename T, typename... Ts>
struct ConsM<T, List<Ts...>> { using value = List<T, Ts...>; };

template<typename T> concept EmptyList = std::same_as<T, List<>>;

template<typename T> concept NonEmptyList =
   requires() { typename T::head; typename T::tail; }
&& std::same_as<T, Cons<typename T::head, typename T::tail>>;

template<typename T> concept AnyList = EmptyList<T> || NonEmptyList<T>;

template<NonEmptyList T> using Head = typename T::head;
template<NonEmptyList T> using Tail = typename T::tail;

template<template<typename...> typename, typename> struct ApplyM;
template<template<typename...> typename M, AnyList Ts>
using Apply = typename ApplyM<M, Ts>::value;

template<template<typename...> typename M, typename... Ts>
struct ApplyM<M, List<Ts...>> { using value = M<Ts...>; };

template<template<typename...> typename, typename, typename> struct Apply1M;
template<template<typename...> typename M, typename T, AnyList Ts>
using Apply1 = typename Apply1M<M, T, Ts>::value;

template<template<typename...> typename M, typename T, typename... Ts>
struct Apply1M<M, T, List<Ts...>> { using value = M<T, Ts...>; };

template<template<typename> typename, typename> struct MapM;
template<template<typename> typename φ, AnyList Ts>
using Map = typename MapM<φ, Ts>::value;

template<template<typename> typename φ>
struct MapM<φ, List<>> { using value = List<>; };

template<template<typename> typename φ, typename T, typename... Ts>
struct MapM<φ, List<T, Ts...>> { using value = Cons<φ<T>, Map<φ, List<Ts...>>>; };

template<typename... Ts> struct LengthM
{ constexpr static size_t value = sizeof...(Ts); };

template<AnyList T> constexpr size_t Length = Apply<LengthM, T>::value;

template<int k, typename T> struct EnumerateItem
{ static constexpr int index = k; using typeval = T; };

template<int, typename...> struct EnumerateM;
template<typename... Ts> using Enumerate = typename EnumerateM<0, Ts...>::value;

template<int k> struct EnumerateM<k>
{ using value = List<>; };

template<int k, typename T, typename... Ts> struct EnumerateM<k, T, Ts...>
{ using value = Cons<EnumerateItem<k, T>, typename EnumerateM<k + 1, Ts...>::value>; };
