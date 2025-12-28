#ifndef OPZIONI_VALUE_LIST_HPP
#define OPZIONI_VALUE_LIST_HPP

#include <type_traits>

namespace opz {

template <typename T, T...>
struct ValueList;

// +-------------------------------+
// |          InValueList          |
// +-------------------------------+

template <typename T, T, typename...>
struct InValueList : std::false_type {};

template <typename T, T Needle, T... Haystack>
struct InValueList<T, Needle, ValueList<T, Needle, Haystack...>> : std::true_type {};

template <typename T, T Needle, T Other, T... Haystack>
struct InValueList<T, Needle, ValueList<T, Other, Haystack...>> : InValueList<T, Needle, ValueList<T, Haystack...>> {};

// +----------------------------------+
// |           IndexOfValue           |
// +----------------------------------+

// base case (value not found)
template <int, typename T, T, typename...>
struct IndexOfValue : std::integral_constant<int, -1> {};

// base case (value found)
template <int Idx, typename T, T Needle, T... Haystack>
struct IndexOfValue<Idx, T, Needle, ValueList<T, Needle, Haystack...>> : std::integral_constant<int, Idx> {};

// recursive case (keep looking)
template <int Idx, typename T, T Needle, T Other, T... Haystack>
struct IndexOfValue<Idx, T, Needle, ValueList<T, Other, Haystack...>> : IndexOfValue<Idx + 1, T, Needle, ValueList<T, Haystack...>> {
};

} // namespace opz

#endif // OPZIONI_VALUE_LIST_HPP
