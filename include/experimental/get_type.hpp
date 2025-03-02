#ifndef OPZIONI_GET_TYPE_H
#define OPZIONI_GET_TYPE_H

#include "experimental/string_list.hpp"
#include "experimental/type_list.hpp"

template <typename T>
struct TypeResult {
  using type = T;
};

// base case (type not found)
template <fixed_string, typename...>
struct GetType : TypeResult<void> {};

// base case (type found)
template <fixed_string Needle, fixed_string... Haystack, typename NeedleType, typename... HaystackTypes>
struct GetType<Needle, StringList<Needle, Haystack...>, TypeList<NeedleType, HaystackTypes...>>
    : TypeResult<NeedleType> {};

// recursion case (keep looking)
template <fixed_string Needle, fixed_string Other, fixed_string... Haystack, typename OtherType,
          typename... HaystackTypes>
struct GetType<Needle, StringList<Other, Haystack...>, TypeList<OtherType, HaystackTypes...>>
    : GetType<Needle, StringList<Haystack...>, TypeList<HaystackTypes...>> {};

#endif // OPZIONI_GET_TYPE_H
