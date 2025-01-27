#include <cstddef>

#include "fixed_string.hpp"

template <fixed_string...>
struct StringList;

template <fixed_string, typename...>
struct InStringList : std::false_type {};

template <fixed_string Needle, fixed_string... Haystack>
struct InStringList<Needle, StringList<Needle, Haystack...>> : std::true_type {};

template <fixed_string Needle, fixed_string Other, fixed_string... Haystack>
struct InStringList<Needle, StringList<Other, Haystack...>> : InStringList<Needle, StringList<Haystack...>> {};

template <typename...>
struct TypeList;

template<std::size_t, typename... Ts>
struct IndexOfImpl : std::integral_constant<std::size_t, sizeof... (Ts)> {};

template<std::size_t Idx, typename T, typename... Ts>
struct IndexOfImpl<Idx, T, TypeList<T, Ts...>> : std::integral_constant<std::size_t, Idx> {};

template<std::size_t Idx, typename T, typename U, typename... Ts>
struct IndexOfImpl<Idx, T, TypeList<U, Ts...>> : IndexOfImpl<Idx + 1, T, TypeList<Ts...>> {};

template<typename T, typename... Ts>
struct IndexOf : IndexOfImpl<0, T, TypeList<Ts...>> {};

template <typename T>
struct TypeResult {
  using type = T;
};

// base case (type not found)
template <fixed_string, typename...>
struct GetType : TypeResult<void> {};

// base case (type found)
template <fixed_string Needle, fixed_string... Haystack, typename NeedleType, typename... HaystackTypes>
struct GetType<Needle, StringList<Needle, Haystack...>, TypeList<NeedleType, HaystackTypes...>> : TypeResult<NeedleType> {};

// recursion case (keep looking)
template <fixed_string Needle, fixed_string Other, fixed_string... Haystack, typename OtherType, typename... HaystackTypes>
struct GetType<Needle, StringList<Other, Haystack...>, TypeList<OtherType, HaystackTypes...>>
  : GetType<Needle, StringList<Haystack...>, TypeList<HaystackTypes...>> {};
