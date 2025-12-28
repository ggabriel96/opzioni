#ifndef OPZIONI_CMD_HPP
#define OPZIONI_CMD_HPP

#include <functional>
#include <optional>
#include <tuple>

#include "opzioni/arg.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/fixed_string.hpp"
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

template <
  FixedString... Names,
  FixedString... Abbrevs,
  ArgKind... Kinds,
  typename... Types,
  typename... Tags,
  concepts::Cmd... SubCmds>
struct Cmd<
  StringList<Names...>,
  StringList<Abbrevs...>,
  ArgKindList<Kinds...>,
  TypeList<Types...>,
  TypeList<Tags...>,
  TypeList<SubCmds...>> {
  using arg_names = StringList<Names...>;
  using arg_abbrevs = StringList<Abbrevs...>;
  using arg_kinds = ArgKindList<Kinds...>;
  using arg_types = TypeList<Types...>;
  using arg_tags = TypeList<Tags...>;
  using subcmd_types = TypeList<SubCmds...>;
  // using amount_pos = std::integral_constant<std::size_t, (0 + ... + static_cast<std::size_t>(Kinds == ArgKind::POS))>;

  std::string_view name{};
  std::string_view version{};
  std::string_view introduction{};
  std::size_t msg_width{100};
  ErrorHandler error_handler{print_error_and_usage};

  std::tuple<Arg<Types, Tags> const...> args;
  std::tuple<std::reference_wrapper<SubCmds const> const...> subcmds;

  consteval Cmd() = default;
  explicit consteval Cmd(std::string_view const name, std::string_view const version = "")
    : name(name), version(version) {
    if (!is_valid_name(name)) throw "Command names must neither be empty nor contain any whitespace";
  }

  template <concepts::Cmd OtherCmd>
  explicit consteval Cmd(OtherCmd const &other)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      args(other.args),
      subcmds(other.subcmds) {}

  template <concepts::Cmd OtherCmd, typename T, typename Tag>
  consteval Cmd(OtherCmd const &other, Arg<T, Tag> const new_arg)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      args(std::tuple_cat(other.args, std::make_tuple(new_arg))),
      subcmds(other.subcmds) {}

  template <concepts::Cmd OtherCmd, concepts::Cmd NewSubCmd>
  consteval Cmd(OtherCmd const &other, NewSubCmd const &new_subcmd)
    : name(other.name),
      version(other.version),
      introduction(other.introduction),
      msg_width(other.msg_width),
      error_handler(other.error_handler),
      args(other.args),
      subcmds(std::tuple_cat(other.subcmds, std::make_tuple(std::cref(new_subcmd)))) {}

  consteval auto intro(std::string_view const intro) {
    if (!is_valid_intro(intro))
      throw "Command intros, if specified, must neither be empty nor start or end with whitespace";
    this->introduction = intro;
    return *this;
  }

  consteval auto with(ExtraConfig const cfg) {
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
  consteval auto sub(NewSubCmd const &subcmd) const {
    static_assert(
      !InArgKindList<ArgKind::POS, arg_kinds>::value, "Commands that have positional arguments cannot have subcommands"
    );
    if (auto const existing_cmd_idx = find_cmd(subcmds, subcmd.name); existing_cmd_idx != -1)
      throw "Subcommand with this name already exists";
    Cmd<
      StringList<Names...>,
      StringList<Abbrevs...>,
      ArgKindList<Kinds...>,
      TypeList<Types...>,
      TypeList<Tags...>,
      TypeList<SubCmds..., NewSubCmd>>
      new_cmd(*this, subcmd);
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view, typename Tag = act::assign>
  consteval auto pos(ArgMeta<T, Tag> const meta) const {
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    static_assert(!this->has_subcmds(), "Commands that have subcommands cannot have positional arguments");
    validate_common<Name, "">(meta);
    validate_pos(meta);
    Cmd<
      StringList<Names..., Name>,
      StringList<Abbrevs..., "">,
      ArgKindList<Kinds..., ArgKind::POS>,
      TypeList<Types..., T>,
      TypeList<Tags..., Tag>,
      TypeList<SubCmds...>>
      new_cmd(
        *this,
        Arg<T, Tag>{
          .kind = ArgKind::POS,
          .name = Name,
          .abbrev = "",
          .help = meta.help,
          .is_required = meta.is_required.value_or(true),
          .default_value = meta.default_value,
          .implicit_value = std::nullopt,
        }
      );
    return new_cmd;
  }

  template <FixedString Name, FixedString Abbrev, typename T = std::string_view, typename Tag = act::assign>
  consteval auto opt(ArgMeta<T, Tag> const meta) const {
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    if constexpr (Abbrev.size > 0) {
      static_assert(!InStringList<Abbrev, arg_abbrevs>::value, "Argument with this abbreviation already exists");
      static_assert(Abbrev.size == 1, "Abbreviations must be a single character");
      static_assert(Abbrev[0] >= 'A' || Abbrev[0] <= 'Z', "Option abbreviations must be an uppercase Latin letter");
    }
    validate_common<Name, Abbrev>(meta);
    validate_opt(meta);
    Cmd<
      StringList<Names..., Name>,
      StringList<Abbrevs..., Abbrev>,
      ArgKindList<Kinds..., ArgKind::OPT>,
      TypeList<Types..., T>,
      TypeList<Tags..., Tag>,
      TypeList<SubCmds...>>
      new_cmd(
        *this,
        Arg<T, Tag>{
          .kind = ArgKind::OPT,
          .name = Name,
          .abbrev = Abbrev,
          .help = meta.help,
          .is_required = meta.is_required.value_or(false),
          .default_value = meta.default_value.value_or(T{}),
          .implicit_value = meta.implicit_value,
        }
      );
    return new_cmd;
  }

  template <FixedString Name, typename T = std::string_view, typename Tag = act::assign>
  consteval auto opt(ArgMeta<T, Tag> const meta) const {
    return opt<Name, "", T, Tag>(meta);
  }

  template <FixedString Name, FixedString Abbrev, typename T = bool, typename Tag = act::assign>
  consteval auto flg(ArgMeta<T, Tag> const meta) const {
    static_assert(!InStringList<Name, arg_names>::value, "Argument with this name already exists");
    if constexpr (Abbrev.size > 0) {
      static_assert(!InStringList<Abbrev, arg_abbrevs>::value, "Argument with this abbreviation already exists");
      static_assert(Abbrev.size == 1, "Abbreviations must be a single character");
      static_assert(Abbrev[0] >= 'a' || Abbrev[0] <= 'z', "Flag abbreviations must be a lowercase Latin letter");
    }
    validate_common<Name, Abbrev>(meta);
    validate_flg(meta);
    std::optional<T> default_implicit_value = std::nullopt;
    if constexpr (std::is_same_v<T, bool>) default_implicit_value.emplace(true);
    else if constexpr (concepts::Integer<T>) default_implicit_value.emplace(T{});
    Cmd<
      StringList<Names..., Name>,
      StringList<Abbrevs..., Abbrev>,
      ArgKindList<Kinds..., ArgKind::FLG>,
      TypeList<Types..., T>,
      TypeList<Tags..., Tag>,
      TypeList<SubCmds...>>
      new_cmd(
        *this,
        Arg<T, Tag>{
          .kind = ArgKind::FLG,
          .name = Name,
          .abbrev = Abbrev,
          .help = meta.help,
          .is_required = false,
          .default_value = meta.default_value.value_or(T{}),
          // the following dereference will not crash because we are validating non-bool non-int flags have
          // implicit_value
          .implicit_value = meta.implicit_value.value_or(*default_implicit_value),
        }
      );
    return new_cmd;
  }

  template <FixedString Name, typename T = bool, typename Tag = act::assign>
  consteval auto flg(ArgMeta<T, Tag> const meta) const {
    return flg<Name, "", T, Tag>(meta);
  }

  auto operator()(int const argc, char const *argv[]) const noexcept {
    try {
      auto parser = CmdParser(*this);
      return parser(argc, argv);
    } catch (UserError &ue) {
      std::exit(this->error_handler(ue));
    }
  }

  [[nodiscard]] constexpr bool has_subcmds() const noexcept { return std::tuple_size_v<decltype(this->subcmds)> > 0; }
};

consteval auto new_cmd(std::string_view const name, std::string_view const version = "") {
  return Cmd<StringList<>, StringList<>, ArgKindList<>, TypeList<>, TypeList<>, TypeList<>>(name, version);
}

} // namespace opz

#endif // OPZIONI_CMD_HPP
