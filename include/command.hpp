#ifndef OPZIONI_COMMAND_H
#define OPZIONI_COMMAND_H

#include <optional>
#include <tuple>

#include "arg.hpp"
#include "concepts.hpp"
#include "fixed_string.hpp"
#include "get_type.hpp"

namespace opz {

template <typename...>
struct Command;

template <FixedString... Names, typename... Types, concepts::Command... SubCmds>
struct Command<StringList<Names...>, TypeList<Types...>, SubCmds...> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;
  using subcmd_types = TypeList<SubCmds...>;

  std::string_view name{};
  std::string_view version{};
  std::string_view introduction{};
  std::tuple<Arg<Types>...> args;
  std::size_t amount_pos = 0;

  std::size_t msg_width = 100; // TODO: expose

  // TODO: make it not store whole objects in the tuple (reference_wrapper?)
  // TODO: make them const?
  std::tuple<SubCmds...> subcmds;

  consteval Command() = default;
  explicit consteval Command(std::string_view name, std::string_view version = "") : name(name), version(version) {}

  template <concepts::Command Cmd>
  consteval Command(Cmd const &other) {
    name = other.name;
    version = other.version;
    introduction = other.introduction;
    amount_pos = other.amount_pos;
    if constexpr (std::tuple_size_v<decltype(args)> == std::tuple_size_v<decltype(other.args)>) args = other.args;
    if constexpr (std::tuple_size_v<decltype(subcmds)> == std::tuple_size_v<decltype(other.subcmds)>)
      subcmds = other.subcmds;
  }

  consteval auto intro(std::string_view intro) const noexcept {
    auto p = *this;
    p.introduction = intro;
    return p;
  }

  template <concepts::Command NewSubCmd>
  consteval auto sub(NewSubCmd const &subcmd) const noexcept {
    Command<StringList<Names...>, TypeList<Types...>, SubCmds..., NewSubCmd> new_cmd(*this);
    new_cmd.subcmds = std::tuple_cat(subcmds, std::make_tuple(subcmd));
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view>
  consteval auto pos(ArgMeta<T> meta) {
    if (meta.is_required.value_or(false) && meta.default_value.has_value())
      throw "Required arguments cannot have default values";
    if (meta.implicit_value.has_value())
      throw "Positionals cannot use implicit value because they always take a value from the command-line";
    if (meta.action == Action::COUNT)
      throw "Positionals cannot use the COUNT action because they always take a value from the command-line";

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
        .implicit_value = std::nullopt,
        .action = meta.action,
      }));
    new_cmd.amount_pos += 1;
    return new_cmd;
  }

  template <FixedString Name, FixedString Abbrev, typename T = std::string_view>
  consteval auto opt(ArgMeta<T> meta) {
    // TODO: add thorough validations
    static_assert(Abbrev.size <= 1, "Abbreviations must be a single character");
    if (meta.is_required.value_or(false) && meta.default_value.has_value())
      throw "Required arguments cannot have default values";
    if (meta.action == Action::COUNT) {
      if (!concepts::Integer<T>) throw "The COUNT action only works with integer types";
      if (!meta.implicit_value.has_value()) throw "The COUNT action requires an implicit value";
    }
    if (meta.action == Action::APPEND || meta.action == Action::CSV) {
      if (!concepts::Container<T>)
        throw "The APPEND and CSV actions require that the argument type satisfy the opz::concepts::Container concept";
      if (meta.implicit_value.has_value())
        throw "The APPEND and CSV actions do not work with implicit value since they require a value from the "
              "command-line";
    }

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
        .implicit_value = meta.implicit_value,
        .action = meta.action,
      }));
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view>
  consteval auto opt(ArgMeta<T> meta) {
    return opt<Name, "", T>(meta);
  }

  template <FixedString Name, FixedString Abbrev, typename T = bool>
  consteval auto flg(ArgMeta<T> meta) {
    static_assert(Abbrev.size <= 1, "Abbreviations must be a single character");
    if (meta.is_required.value_or(false)) throw "Flags cannot be required";
    // TODO: can we try to be smart about default implicit values of other types?
    if (!std::is_same_v<T, bool> && !meta.implicit_value)
      throw "Non-boolean flags require that the implicit value is specified";
    if (meta.action == Action::COUNT) {
      if (!concepts::Integer<T>) throw "The COUNT action only works with integer types";
      if (!meta.implicit_value.has_value()) throw "The COUNT action requires an implicit value";
    }

    Command<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(Arg<T>{
        .type = ArgType::FLG,
        .name = Name,
        .abbrev = Abbrev,
        .help = meta.help,
        .is_required = false,
        .default_value = meta.default_value.value_or(T{}),
        .implicit_value = meta.implicit_value.value_or(true),
        .action = meta.action,
      }));
    return new_cmd;
  }

  template <FixedString Name, typename T = bool>
  consteval auto flg(ArgMeta<T> meta) {
    return flg<Name, "", T>(meta);
  }

  // template<FixedString Name>
  // constexpr std::optional<typename GetType<Name, arg_names, arg_types>::type> GetValue() const noexcept {
  //   using ValueIdx = IndexOfStr<0, Name, arg_names>;
  //   static_assert(ValueIdx::value != -1, "unknown parameter name");
  //   return std::get<ValueIdx::value>(values);
  // }

  // template<FixedString Name, typename V>
  // void SetValue(V value) noexcept {
  //   using T = GetType<Name, arg_names, arg_types>::type;
  //   static_assert(!std::is_same_v< T, void >, "unknown parameter name");
  //   static_assert(std::is_same_v< V, T >, "parameter of given name has different type than provided value");
  //   using ValueIdx = IndexOfType<0, V, arg_types>;

  //   std::get<ValueIdx::value>(values).emplace(value);
  // }
};

consteval auto new_cmd(std::string_view name, std::string_view version = "") {
  return Command<StringList<>, TypeList<>>(name, version);
}

} // namespace opz

#endif // OPZIONI_COMMAND_H
