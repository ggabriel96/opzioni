#ifndef OPZIONI_PROGRAM_H
#define OPZIONI_PROGRAM_H

#include <algorithm>
#include <array>
#include <optional>
#include <tuple>
#include <type_traits>

#include "experimental/arg.hpp"
#include "experimental/fixed_string.hpp"
#include "experimental/get_type.hpp"

template<typename ...>
struct Program;

template<>
struct Program<> {

  template <fixed_string Name, typename T>
  consteval auto Pos(ArgSpec spec) {
    Program<StringList<Name>, TypeList<T>> new_program;
    new_program.args[0] = Arg{
      .name = Name,
      .abbrev = "",
      .help = spec.help,
      .is_required = spec.is_required,
    };
    new_program.amount_pos += 1;
    return new_program;
  }

};

template <fixed_string... Names, typename... Types>
struct Program<StringList<Names...>, TypeList<Types...>> {
  using argNames = StringList<Names...>;
  using argTypes = TypeList<Types...>;

  std::array<Arg, sizeof... (Names)> args;
  std::size_t amount_pos = 0;

  consteval Program() = default;

  template <fixed_string... OtherNames, typename... OtherTypes>
  consteval Program(Program<StringList<OtherNames...>, TypeList<OtherTypes...>> const &other) {
    std::copy_n(other.args.begin(), sizeof... (OtherNames), args.begin());
  }

  template <fixed_string Name, typename T>
  consteval auto Pos(ArgSpec spec) {
    Program<StringList<Name, Names...>, TypeList<T, Types...>> new_program(*this);
    new_program.args[sizeof... (Names) - 1] = Arg{
      .name = Name,
      .abbrev = "",
      .help = spec.help,
      .is_required = spec.is_required,
    };
    new_program.amount_pos += 1;
    std::sort(new_program.args.begin(), new_program.args.end());
    return new_program;
  }

  // template<fixed_string Name>
  // constexpr std::optional<typename GetType<Name, argNames, argTypes>::type> GetValue() const noexcept {
  //   using ValueIdx = IndexOfStr<0, Name, argNames>;
  //   static_assert(ValueIdx::value != -1, "unknown parameter name");
  //   return std::get<ValueIdx::value>(values);
  // }

  // template<fixed_string Name, typename V>
  // void SetValue(V value) noexcept {
  //   using T = GetType<Name, argNames, argTypes>::type;
  //   static_assert(!std::is_same_v< T, void >, "unknown parameter name");
  //   static_assert(std::is_same_v< V, T >, "parameter of given name has different type than provided value");
  //   using ValueIdx = IndexOfType<0, V, argTypes>;

  //   std::get<ValueIdx::value>(values).emplace(value);
  // }
};

#endif