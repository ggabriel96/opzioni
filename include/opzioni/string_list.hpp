#ifndef OPZIONI_STRING_LIST_HPP
#define OPZIONI_STRING_LIST_HPP

#include "opzioni/fixed_string.hpp"

namespace opz {

// Note: can't create a specialization of ValueList and friends for FixedString because it itself is a template type

template <FixedString...>
struct StringList;

// +--------------------------------+
// |          InStringList          |
// +--------------------------------+

template <FixedString, typename...>
struct InStringList : std::false_type {};

template <FixedString Needle, FixedString... Haystack>
struct InStringList<Needle, StringList<Needle, Haystack...>> : std::true_type {};

template <FixedString Needle, FixedString Other, FixedString... Haystack>
struct InStringList<Needle, StringList<Other, Haystack...>> : InStringList<Needle, StringList<Haystack...>> {};

// +--------------------------------+
// |           IndexOfStr           |
// +--------------------------------+

// base case (str not found)
template <int Idx, FixedString, typename...>
struct IndexOfStr : std::integral_constant<int, -1> {};

// base case (str found)
template <int Idx, FixedString Needle, FixedString... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Needle, Haystack...>> : std::integral_constant<int, Idx> {};

// recursive case (keep looking)
template <int Idx, FixedString Needle, FixedString Other, FixedString... Haystack>
struct IndexOfStr<Idx, Needle, StringList<Other, Haystack...>> : IndexOfStr<Idx + 1, Needle, StringList<Haystack...>> {
};

} // namespace opz

#endif // OPZIONI_STRING_LIST_HPP
