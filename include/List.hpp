#pragma once

template<typename...> struct List;

template<typename, typename> struct ConsM;
template<typename T, typename Ts>
using Cons = ConsM<T, Ts>::val;

template<typename T, typename... Ts>
struct ConsM<T, List<Ts...>> { using val = List<T, Ts...>; };

template<template<typename> typename, typename> struct MapM;
template<template<typename> typename φ, typename Ts>
using Map = MapM<φ, Ts>::val;

template<template<typename> typename φ>
struct MapM<φ, List<>> { using val = List<>; };

template<template<typename> typename φ, typename T, typename... Ts>
struct MapM<φ, List<T, Ts...>> { using val = Cons<φ<T>, Map<φ, List<Ts...>>>; };

template<template<typename...> typename, typename> struct ApplyM;
template<template<typename...> typename M, typename T>
using Apply = ApplyM<M, T>::val;

template<template<typename...> typename M, typename... Ts>
struct ApplyM<M, List<Ts...>> { using val = M<Ts...>; };