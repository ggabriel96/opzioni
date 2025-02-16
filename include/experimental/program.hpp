#ifndef OPZIONI_PROGRAM_H
#define OPZIONI_PROGRAM_H

#include <array>
#include <optional>
#include <tuple>
#include <type_traits>

#include "experimental/arg.hpp"
#include "experimental/fixed_string.hpp"
#include "experimental/get_type.hpp"

template <typename...>
struct Program;

template <fixed_string... Names, typename... Types>
struct Program<StringList<Names...>, TypeList<Types...>> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;

  std::string_view name{};
  std::string_view version{};
  std::string_view intro{};
  std::array<Arg, sizeof...(Names)> args;
  std::size_t amount_pos = 0;

  consteval Program() = default;
  consteval Program(std::string_view name, std::string_view version = "") : name(name), version(version) {}

  template <fixed_string... OtherNames, typename... OtherTypes>
  consteval Program(Program<StringList<OtherNames...>, TypeList<OtherTypes...>> const &other) {
    name = other.name;
    version = other.version;
    intro = other.intro;
    amount_pos = other.amount_pos;
    std::copy_n(other.args.begin(), sizeof...(OtherNames), args.begin());
  }

  consteval auto Intro(std::string_view intro) const noexcept {
    auto p = *this;
    p.intro = intro;
    return p;
  }

  template <fixed_string Name, typename T>
  consteval auto Pos(ArgMeta meta) {
    Program<StringList<Name, Names...>, TypeList<T, Types...>> new_program(*this);
    new_program.args[sizeof...(Names)] = Arg{
      .type = ArgType::POS,
      .name = Name,
      .abbrev = "",
      .help = meta.help,
      .is_required = meta.is_required.value_or(true),
    };
    new_program.amount_pos += 1;
    return new_program;
  }

  template <fixed_string Name, fixed_string Abbrev, typename T>
  consteval auto Opt(ArgMeta meta) {
    // TODO: can we remove the trailing \n from fixed_string?
    // TODO: add thorough validations
    static_assert(Abbrev.size <= 2, "Abbreviations must be a single character");

    Program<StringList<Name, Names...>, TypeList<T, Types...>> new_program(*this);
    new_program.args[sizeof...(Names)] = Arg{
        .type = ArgType::OPT,
        .name = Name,
        .abbrev = Abbrev,
        .help = meta.help,
        .is_required = meta.is_required.value_or(false),
    };
    return new_program;
  }

  template <fixed_string Name, typename T>
  consteval auto Opt(ArgMeta meta) {
    return Opt<Name, "", T>(meta);
  }

  template <fixed_string Name, fixed_string Abbrev = "">
  consteval auto Flg(ArgMeta meta) {
    Program<StringList<Name, Names...>, TypeList<bool, Types...>> new_program(*this);
    new_program.args[sizeof...(Names)] = Arg{
        .type = ArgType::FLG,
        .name = Name,
        .abbrev = Abbrev,
        .help = meta.help,
        .is_required = false,
    };
    return new_program;
  }

  // template<fixed_string Name>
  // constexpr std::optional<typename GetType<Name, arg_names, arg_types>::type> GetValue() const noexcept {
  //   using ValueIdx = IndexOfStr<0, Name, arg_names>;
  //   static_assert(ValueIdx::value != -1, "unknown parameter name");
  //   return std::get<ValueIdx::value>(values);
  // }

  // template<fixed_string Name, typename V>
  // void SetValue(V value) noexcept {
  //   using T = GetType<Name, arg_names, arg_types>::type;
  //   static_assert(!std::is_same_v< T, void >, "unknown parameter name");
  //   static_assert(std::is_same_v< V, T >, "parameter of given name has different type than provided value");
  //   using ValueIdx = IndexOfType<0, V, arg_types>;

  //   std::get<ValueIdx::value>(values).emplace(value);
  // }
};

consteval auto DefaultProgram(std::string_view name, std::string_view version = "") {
  auto p = Program<StringList<"help">, TypeList<bool>>(name);
  p.args[0] = Arg{
      .type = ArgType::FLG,
      .name = "help",
      .abbrev = "h",
      .help = "Display this information",
      .is_required = false,
  };
  return p;
}

#endif