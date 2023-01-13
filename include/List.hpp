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
using Cons = typename ConsM<T, Ts>::val;

template<typename T, typename... Ts>
struct ConsM<T, List<Ts...>> { using val = List<T, Ts...>; };

template<template<typename...> typename, typename> struct ApplyM;
template<template<typename...> typename M, typename T>
using Apply = typename ApplyM<M, T>::val;

template<template<typename...> typename M, typename... Ts>
struct ApplyM<M, List<Ts...>> { using val = M<Ts...>; };

template<template<typename> typename, typename> struct MapM;
template<template<typename> typename φ, typename Ts>
using Map = typename MapM<φ, Ts>::val;

template<template<typename> typename φ>
struct MapM<φ, List<>> { using val = List<>; };

template<template<typename> typename φ, typename T, typename... Ts>
struct MapM<φ, List<T, Ts...>> { using val = Cons<φ<T>, Map<φ, List<Ts...>>>; };