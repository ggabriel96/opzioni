#include <cstddef>

#include "fixed_string.hpp"

// +--------------------------------+
// |           StringList           |
// +--------------------------------+

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

template<int, typename... Ts>
struct IndexOfTypeImpl : std::integral_constant<int, -1> {};

template<int Idx, typename T, typename... Ts>
struct IndexOfTypeImpl<Idx, T, TypeList<T, Ts...>> : std::integral_constant<int, Idx> {};

template<int Idx, typename T, typename U, typename... Ts>
struct IndexOfTypeImpl<Idx, T, TypeList<U, Ts...>> : IndexOfTypeImpl<Idx + 1, T, TypeList<Ts...>> {};

template<typename T, typename... Ts>
struct IndexOfType : IndexOfTypeImpl<0, T, TypeList<Ts...>> {};

// +-----------------------------+
// |           GetType           |
// +-----------------------------+

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
