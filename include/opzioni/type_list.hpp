#ifndef OPZIONI_TYPE_LIST_HPP
#define OPZIONI_TYPE_LIST_HPP

#include <type_traits>

namespace opz {

template <typename...>
struct TypeList;

// +-----------------------------------+
// |            IndexOfType            |
// +-----------------------------------+

template <int, typename...>
struct IndexOfType : std::integral_constant<int, -1> {};

template <int Idx, typename T, typename... Ts>
struct IndexOfType<Idx, T, TypeList<T, Ts...>> : std::integral_constant<int, Idx> {};

template <int Idx, typename T, typename U, typename... Ts>
struct IndexOfType<Idx, T, TypeList<U, Ts...>> : IndexOfType<Idx + 1, T, TypeList<Ts...>> {};

// +----------------------------------+
// |              InList              |
// +----------------------------------+

template <typename...>
struct InList : std::false_type {};

template <typename T, typename... Ts>
struct InList<T, TypeList<T, Ts...>> : std::true_type {};

template <typename T, typename U, typename... Ts>
struct InList<T, TypeList<U, Ts...>> : InList<T, TypeList<Ts...>> {};

// +----------------------------------+
// |              Concat              |
// +----------------------------------+

template <typename...>
struct Concat;

template <typename... Lhs, typename... Rhs>
struct Concat<TypeList<Lhs...>, TypeList<Rhs...>> {
  using type = TypeList<Lhs..., Rhs...>;
};

} // namespace opz

#endif // OPZIONI_TYPE_LIST_HPP
