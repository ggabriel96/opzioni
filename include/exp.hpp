#include <type_traits>

#include "fixed_string.hpp"
#include "get_type.hpp"

template<typename ...>
struct Program;

template<>
struct Program<> {
  template <fixed_string Name, typename T>
  constexpr auto Add() {
    return Program<StringList<Name>, TypeList<T>>{};
  }
};

template <fixed_string... Names, typename... Types>
struct Program<StringList<Names...>, TypeList<Types...>> {
  using argNames = StringList<Names...>;
  using argTypes = TypeList<Types...>;

  template <fixed_string Name, typename T>
  constexpr auto Add() {
    return Program<StringList<Name, Names...>, TypeList<T, Types...>>{};
  }

  template<fixed_string Name>
  constexpr GetType<Name, argNames, argTypes>::type GetValue() const noexcept {
    using T = GetType<Name, argNames, argTypes>::type;
    static_assert(!std::is_same_v< T, void >, "unknown parameter name");
    return T();
  }
};
