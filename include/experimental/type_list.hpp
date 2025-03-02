#ifndef OPZIONI_TYPE_LIST_H
#define OPZIONI_TYPE_LIST_H

template <typename...>
struct TypeList;

// +-----------------------------------+
// |            IndexOfType            |
// +-----------------------------------+

template <int, typename... Ts>
struct IndexOfType : std::integral_constant<int, -1> {};

template <int Idx, typename T, typename... Ts>
struct IndexOfType<Idx, T, TypeList<T, Ts...>> : std::integral_constant<int, Idx> {};

template <int Idx, typename T, typename U, typename... Ts>
struct IndexOfType<Idx, T, TypeList<U, Ts...>> : IndexOfType<Idx + 1, T, TypeList<Ts...>> {};

#endif // OPZIONI_TYPE_LIST_H
