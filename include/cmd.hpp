#ifndef OPZIONI_COMMAND_H
#define OPZIONI_COMMAND_H

#include <optional>
#include <tuple>

#include "arg.hpp"
#include "concepts.hpp"
#include "fixed_string.hpp"
#include "get_type.hpp"
#include "strings.hpp"

namespace opz {

template <typename...> struct Cmd;

template <FixedString... Names, typename... Types, concepts::Cmd... SubCmds>
struct Cmd<StringList<Names...>, TypeList<Types...>, SubCmds...> {
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
  std::tuple<SubCmds...> subcmds;

  consteval Cmd() = default;
  explicit consteval Cmd(std::string_view name, std::string_view version = "") : name(name), version(version) {
    if (!is_valid_name(name)) throw "Command names must neither be empty nor contain any whitespace";
  }

  template <concepts::Cmd OtherCmd>
  consteval Cmd(OtherCmd const &other) {
    name = other.name;
    version = other.version;
    introduction = other.introduction;
    amount_pos = other.amount_pos;
    if constexpr (std::tuple_size_v<decltype(args)> == std::tuple_size_v<decltype(other.args)>) args = other.args;
    if constexpr (std::tuple_size_v<decltype(subcmds)> == std::tuple_size_v<decltype(other.subcmds)>)
      subcmds = other.subcmds;
  }

  consteval auto intro(std::string_view intro) {
    if (!is_valid_intro(intro))
      throw "Command intros, if specified, must neither be empty nor start or end with whitespace";
    this->introduction = intro;
    return *this;
  }

  template <concepts::Cmd NewSubCmd>
  consteval auto sub(NewSubCmd const &subcmd) const noexcept {
    Cmd<StringList<Names...>, TypeList<Types...>, SubCmds..., NewSubCmd> new_cmd(*this);
    new_cmd.subcmds = std::tuple_cat(subcmds, std::make_tuple(subcmd));
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view>
  consteval auto pos(ArgMeta<T> meta) {
    validate_common<Name, "">(meta);
    validate_pos(meta);
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(
        Arg<T>{
          .type = ArgType::POS,
          .name = Name,
          .abbrev = "",
          .help = meta.help,
          .is_required = meta.is_required.value_or(true),
          .default_value = meta.default_value,
          .implicit_value = std::nullopt,
          .action = meta.action,
        }
      )
    );
    new_cmd.amount_pos += 1;
    return new_cmd;
  }

  template <FixedString Name, FixedString Abbrev, typename T = std::string_view>
  consteval auto opt(ArgMeta<T> meta) {
    validate_common<Name, Abbrev>(meta);
    validate_opt(meta);
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(
        Arg<T>{
          .type = ArgType::OPT,
          .name = Name,
          .abbrev = Abbrev,
          .help = meta.help,
          .is_required = meta.is_required.value_or(false),
          .default_value = meta.default_value,
          .implicit_value = meta.implicit_value,
          .action = meta.action,
        }
      )
    );
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view>
  consteval auto opt(ArgMeta<T> meta) {
    return opt<Name, "", T>(meta);
  }

  template <FixedString Name, FixedString Abbrev, typename T = bool>
  consteval auto flg(ArgMeta<T> meta) {
    validate_common<Name, Abbrev>(meta);
    validate_flg(meta);
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>> new_cmd(*this);
    new_cmd.args = std::tuple_cat(
      args,
      std::make_tuple(
        Arg<T>{
          .type = ArgType::FLG,
          .name = Name,
          .abbrev = Abbrev,
          .help = meta.help,
          .is_required = false,
          .default_value = meta.default_value.value_or(T{}),
          .implicit_value = meta.implicit_value.value_or(true),
          .action = meta.action,
        }
      )
    );
    return new_cmd;
  }

  template <FixedString Name, typename T = bool>
  consteval auto flg(ArgMeta<T> meta) {
    return flg<Name, "", T>(meta);
  }
};

consteval auto new_cmd(std::string_view name, std::string_view version = "") {
  return Cmd<StringList<>, TypeList<>>(name, version);
}

} // namespace opz

#endif // OPZIONI_COMMAND_H
