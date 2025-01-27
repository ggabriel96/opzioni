#include <cstddef>

#include "experimental/fixed_string.hpp"

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

// base case (str not found)
template<int Idx, fixed_string, typename...>
struct IndexOfStr : std::integral_constant<int, -1> {};

// base case (str found)
template<int Idx, fixed_string Needle, fixed_string... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Needle, Haystack...>> : std::integral_constant<int, Idx> {};

// recursive case (keep looking)
template<int Idx, fixed_string Needle, fixed_string Other, fixed_string... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Other, Haystack...>> : IndexOfStr<Idx + 1, Needle, StringList<Haystack...>> {};

// +------------------------------+
// |           TypeList           |
// +------------------------------+


template <typename...>
struct TypeList;

template<int, typename... Ts>
struct IndexOfType : std::integral_constant<int, -1> {};

template<int Idx, typename T, typename... Ts>
struct IndexOfType<Idx, T, TypeList<T, Ts...>> : std::integral_constant<int, Idx> {};

template<int Idx, typename T, typename U, typename... Ts>
struct IndexOfType<Idx, T, TypeList<U, Ts...>> : IndexOfType<Idx + 1, T, TypeList<Ts...>> {};

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
