#ifndef OPZIONI_COMMAND_H
#define OPZIONI_COMMAND_H

#include <optional>
#include <tuple>

#include "arg.hpp"
#include "fixed_string.hpp"
#include "get_type.hpp"

namespace opz {

template <typename...>
struct Command;

template <fixed_string... Names, typename... Types>
struct Command<StringList<Names...>, TypeList<Types...>> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;

  std::string_view name{};
  std::string_view version{};
  std::string_view intro{};
  std::tuple<Arg<Types>...> args;
  std::size_t amount_pos = 0;

  consteval Command() = default;
  consteval Command(std::string_view name, std::string_view version = "") : name(name), version(version) {}

  template <fixed_string... OtherNames, typename... OtherTypes>
  consteval Command(Command<StringList<OtherNames...>, TypeList<OtherTypes...>> const &other) {
    name = other.name;
    version = other.version;
    intro = other.intro;
    amount_pos = other.amount_pos;
    if constexpr (sizeof...(Names) == sizeof...(OtherNames)) args = other.args;
  }

  consteval auto Intro(std::string_view intro) const noexcept {
    auto p = *this;
    p.intro = intro;
    return p;
  }

  template <fixed_string Name, typename T>
  consteval auto Pos(ArgMeta<T> meta) {
    Command<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(Arg<T>{
        .type = ArgType::POS,
        .name = Name,
        .abbrev = "",
        .help = meta.help,
        .is_required = meta.is_required.value_or(true),
        .default_value = meta.default_value,
      }));
    new_cmd.amount_pos += 1;
    return new_cmd;
  }

  template <fixed_string Name, fixed_string Abbrev, typename T>
  consteval auto Opt(ArgMeta<T> meta) {
    // TODO: can we remove the trailing \n from fixed_string?
    // TODO: add thorough validations
    static_assert(Abbrev.size <= 2, "Abbreviations must be a single character");

    Command<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(Arg<T>{
        .type = ArgType::OPT,
        .name = Name,
        .abbrev = Abbrev,
        .help = meta.help,
        .is_required = meta.is_required.value_or(false),
        .default_value = meta.default_value,
      }));
    return new_cmd;
  }

  template <fixed_string Name, typename T>
  consteval auto Opt(ArgMeta<T> meta) {
    return Opt<Name, "", T>(meta);
  }

  template <fixed_string Name, fixed_string Abbrev = "">
  consteval auto Flg(ArgMeta<bool> meta) {
    Command<StringList<Names..., Name>, TypeList<Types..., bool>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(Arg<bool>{
        .type = ArgType::FLG,
        .name = Name,
        .abbrev = Abbrev,
        .help = meta.help,
        .is_required = false,
        .default_value = meta.default_value.value_or(false),
      }));
    return new_cmd;
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

consteval auto NewCmd(std::string_view name, std::string_view version = "") {
  auto p = Command<StringList<"help">, TypeList<bool>>(name, version);
  std::get<0>(p.args) = Arg<bool>{
    .type = ArgType::FLG,
    .name = "help",
    .abbrev = "h",
    .help = "Display this information",
    .is_required = false,
  };
  return p;
}

} // namespace opz

#endif // OPZIONI_COMMAND_H
