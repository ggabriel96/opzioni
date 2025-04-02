#ifndef OPZIONI_COMMAND_HPP
#define OPZIONI_COMMAND_HPP

#include <functional>
#include <optional>
#include <tuple>

#include "opzioni/arg.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/fixed_string.hpp"
#include "opzioni/get_type.hpp"
#include "opzioni/parsing.hpp"
#include "opzioni/strings.hpp"

namespace opz {

int print_error(UserError &) noexcept;
int print_error_and_usage(UserError &) noexcept;
int rethrow(UserError &);

using ErrorHandler = int (*)(UserError &);

struct ExtraConfig {
  std::optional<std::size_t> msg_width{};
  std::optional<ErrorHandler> error_handler{};
};

template <typename...> struct Cmd;

template <FixedString... Names, typename... Types, concepts::Cmd... SubCmds>
struct Cmd<StringList<Names...>, TypeList<Types...>, TypeList<SubCmds...>> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;
  using subcmd_types = TypeList<SubCmds...>;

  std::string_view name{};
  std::string_view version{};
  std::string_view introduction{};
  std::size_t msg_width{100};
  ErrorHandler error_handler{print_error_and_usage};

  std::size_t amount_pos{0};
  std::tuple<Arg<Types> const...> args;
  std::tuple<std::reference_wrapper<SubCmds const> const...> subcmds;

  consteval Cmd() = default;
  explicit consteval Cmd(std::string_view name, std::string_view version = "") : name(name), version(version) {
    if (!is_valid_name(name)) throw "Command names must neither be empty nor contain any whitespace";
  }

  template <concepts::Cmd OtherCmd>
  explicit consteval Cmd(OtherCmd const &other)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      amount_pos(other.amount_pos),
      args(other.args),
      subcmds(other.subcmds) {}

  template <concepts::Cmd OtherCmd, typename T>
  explicit consteval Cmd(OtherCmd const &other, Arg<T> new_arg)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      amount_pos(other.amount_pos + static_cast<std::size_t>(new_arg.type == ArgType::POS)),
      args(std::tuple_cat(other.args, std::make_tuple(new_arg))),
      subcmds(other.subcmds) {}

  template <concepts::Cmd OtherCmd, concepts::Cmd NewSubCmd>
  explicit consteval Cmd(OtherCmd const &other, NewSubCmd const &new_subcmd)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      amount_pos(other.amount_pos),
      args(other.args),
      subcmds(std::tuple_cat(other.subcmds, std::make_tuple(std::cref(new_subcmd)))) {}

  consteval auto intro(std::string_view intro) {
    if (!is_valid_intro(intro))
      throw "Command intros, if specified, must neither be empty nor start or end with whitespace";
    this->introduction = intro;
    return *this;
  }

  consteval auto with(ExtraConfig cfg) {
    if (cfg.msg_width.has_value()) {
      if (*cfg.msg_width == 0) throw "The message width must be greater than zero";
      this->msg_width = *cfg.msg_width;
    }
    if (cfg.error_handler.has_value()) {
      if (*cfg.error_handler == nullptr) throw "The error handler cannot be null";
      this->error_handler = *cfg.error_handler;
    }
    return *this;
  }

  template <concepts::Cmd NewSubCmd>
  consteval auto sub(NewSubCmd const &subcmd) const noexcept {
    // TODO: check these subcmds don't exist yet (by name)
    Cmd<StringList<Names...>, TypeList<Types...>, TypeList<SubCmds..., NewSubCmd>> new_cmd(*this, subcmd);
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view>
  consteval auto pos(ArgMeta<T> meta) {
    validate_common<Name, "">(meta);
    validate_pos(meta);
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>, TypeList<SubCmds...>> new_cmd(
      *this,
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
    );
    return new_cmd;
  }

  template <FixedString Name, FixedString Abbrev, typename T = std::string_view>
  consteval auto opt(ArgMeta<T> meta) {
    validate_common<Name, Abbrev>(meta);
    validate_opt(meta);
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    if (Abbrev.size > 0) {
      auto const existing_arg = find_arg_if(args, [](auto const &arg) { return arg.abbrev == Abbrev; });
      if (existing_arg.has_value()) throw "Argument with this abbreviation already exists";
    }
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>, TypeList<SubCmds...>> new_cmd(
      *this,
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
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    if (Abbrev.size > 0) {
      auto const existing_arg = find_arg_if(args, [](auto const &arg) { return arg.abbrev == Abbrev; });
      if (existing_arg.has_value()) throw "Argument with this abbreviation already exists";
    }
    Cmd<StringList<Names..., Name>, TypeList<Types..., T>, TypeList<SubCmds...>> new_cmd(
      *this,
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
    );
    return new_cmd;
  }

  template <FixedString Name, typename T = bool>
  consteval auto flg(ArgMeta<T> meta) {
    return flg<Name, "", T>(meta);
  }

  auto operator()(int argc, char const *argv[]) const noexcept {
    try {
      auto parser = CmdParser(*this);
      return parser(argc, argv);
    } catch (UserError &ue) {
      std::exit(this->error_handler(ue));
    }
  }
};

consteval auto new_cmd(std::string_view name, std::string_view version = "") {
  return Cmd<StringList<>, TypeList<>, TypeList<>>(name, version);
}

} // namespace opz

#endif // OPZIONI_COMMAND_HPP
