#include <optional>
#include <tuple>
#include <type_traits>

#include "fixed_string.hpp"
#include "get_type.hpp"

template<typename ...>
struct Program;

template<>
struct Program<> {
  template <fixed_string Name, typename T>
  consteval auto Add() {
    return Program<StringList<Name>, TypeList<T>>{};
  }
};

template <fixed_string... Names, typename... Types>
struct Program<StringList<Names...>, TypeList<Types...>> {
  using argNames = StringList<Names...>;
  using argTypes = TypeList<Types...>;

  std::tuple<std::optional<Types>...> values{};

  template <fixed_string Name, typename T>
  consteval auto Add() {
    return Program<StringList<Name, Names...>, TypeList<T, Types...>>{};
  }

  template<fixed_string Name>
  constexpr std::optional<typename GetType<Name, argNames, argTypes>::type> GetValue() const noexcept {
    using ValueIdx = IndexOfStr<0, Name, argNames>;
    static_assert(ValueIdx::value != -1, "unknown parameter name");
    return std::get<ValueIdx::value>(values);
  }

  template<fixed_string Name, typename V>
  void SetValue(V value) noexcept {
    using T = GetType<Name, argNames, argTypes>::type;
    static_assert(!std::is_same_v< T, void >, "unknown parameter name");
    static_assert(std::is_same_v< V, T >, "parameter of given name has different type than provided value");
    using ValueIdx = IndexOfType<0, V, argTypes>;

    std::get<ValueIdx::value>(values).emplace(value);
  }
};
