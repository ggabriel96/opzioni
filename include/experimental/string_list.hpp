#ifndef OPZIONI_STRING_LIST_H
#define OPZIONI_STRING_LIST_H

#include "experimental/fixed_string.hpp"

template <fixed_string...>
struct StringList;

// +--------------------------------+
// |          InStringList          |
// +--------------------------------+

template <fixed_string, typename...>
struct InStringList : std::false_type {};

template <fixed_string Needle, fixed_string... Haystack>
struct InStringList<Needle, StringList<Needle, Haystack...>> : std::true_type {};

template <fixed_string Needle, fixed_string Other, fixed_string... Haystack>
struct InStringList<Needle, StringList<Other, Haystack...>> : InStringList<Needle, StringList<Haystack...>> {};

// +--------------------------------+
// |           IndexOfStr           |
// +--------------------------------+

// base case (str not found)
template<int Idx, fixed_string, typename...>
struct IndexOfStr : std::integral_constant<int, -1> {};

// base case (str found)
template<int Idx, fixed_string Needle, fixed_string... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Needle, Haystack...>> : std::integral_constant<int, Idx> {};

// recursive case (keep looking)
template<int Idx, fixed_string Needle, fixed_string Other, fixed_string... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Other, Haystack...>> : IndexOfStr<Idx + 1, Needle, StringList<Haystack...>> {};

#endif