#ifndef OPZIONI_GET_TYPE_H
#define OPZIONI_GET_TYPE_H

#include "string_list.hpp"
#include "type_list.hpp"

namespace opz {

template <typename T>
struct TypeResult {
  using type = T;
};

// base case (type not found)
template <FixedString, typename...>
struct GetType : TypeResult<void> {};

// base case (type found)
template <FixedString Needle, FixedString... Haystack, typename NeedleType, typename... HaystackTypes>
struct GetType<Needle, StringList<Needle, Haystack...>, TypeList<NeedleType, HaystackTypes...>>
    : TypeResult<NeedleType> {};

// recursion case (keep looking)
template <
  FixedString Needle,
  FixedString Other,
  FixedString... Haystack,
  typename OtherType,
  typename... HaystackTypes>
struct GetType<Needle, StringList<Other, Haystack...>, TypeList<OtherType, HaystackTypes...>>
    : GetType<Needle, StringList<Haystack...>, TypeList<HaystackTypes...>> {};

} // namespace opz

#endif // OPZIONI_GET_TYPE_H
