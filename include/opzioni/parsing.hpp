#ifndef OPZIONI_PARSING_HPP
#define OPZIONI_PARSING_HPP

#include <algorithm>
#include <cctype>
#include <functional>
#include <print>
#include <span>
#include <string_view>
#include <vector>

#include "opzioni/args_map.hpp"
#include "opzioni/cmd_info.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/variant.hpp"

namespace opz {

// +---------------------+
// |      fwd decls      |
// +---------------------+

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view get_if_short_flags(std::string_view const) noexcept;
constexpr std::string_view get_if_long_flag(std::string_view const) noexcept;

struct ArgView {
  std::size_t tuple_idx{};
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  bool is_required = false;
  bool has_implicit = false;

  template <typename T>
  constexpr static ArgView from(std::size_t tuple_idx, Arg<T> const &other) {
    return ArgView{
      .tuple_idx = tuple_idx,
      .type = other.type,
      .name = other.name,
      .abbrev = other.abbrev,
      .is_required = other.is_required,
      .has_implicit = other.implicit_value.has_value()
    };
  }
};

template <typename... Ts>
constexpr auto find_arg_if(std::tuple<Arg<Ts> const...> const haystack, std::predicate<ArgView> auto p) {
  // TODO: use something like the frozen library instead of this
  return std::apply(
    [&p](auto &&...elem) {
      std::size_t idx = 0;
      std::optional<ArgView> ret = std::nullopt;
      // clang-format off
        (void) // cast to void to suppress unused warning
        ((p(elem) ? (ret = ArgView::from(idx, elem), true) : (++idx, false)) || ...);
      // clang-format on
      return ret;
    },
    haystack
  );
}

template <concepts::Cmd... Cmds>
constexpr int
find_cmd(std::tuple<std::reference_wrapper<Cmds const> const...> const haystack, std::string_view const name) {
  // TODO: use something like the frozen library instead of this
  // clang-format off
  return std::apply(
    [name](auto &&...elem) {
      int idx = 0, ret = -1;
      (void) // cast to void to suppress unused warning
      ((elem.get().name == name ? (ret = idx, true) : (++idx, false)) || ...);
      return ret;
    },
    haystack);
  // clang-format on
}

struct ParsedOption {
  ArgView arg;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

// +-----------------------+
// |   main parsing code   |
// +-----------------------+

template <concepts::Cmd Cmd>
class CmdParser {
public:
  using cmd_type = Cmd;
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;
  using subcmd_types = typename Cmd::subcmd_types;

  std::reference_wrapper<Cmd const> cmd_ref;
  CmdInfoGetter info;

  explicit CmdParser(Cmd const &cmd) : CmdParser(cmd, "") {}

  ArgsMap<Cmd const> operator()(std::span<char const *> args) {
    auto map = this->get_args_map(args);
    // check_contains_required done separately in order to report all missing required args
    this->check_contains_required(map);
    this->set_defaults(map);
    return map;
  }

  ArgsMap<Cmd const> operator()(int argc, char const *argv[]) {
    auto const args = std::span{argv, static_cast<std::size_t>(argc)};
    return (*this)(args);
  }

private:
  template <concepts::Cmd>
  friend class CmdParser;

  CmdParser(Cmd const &cmd, std::string const &parent_cmds_names) : cmd_ref(cmd), info(cmd, parent_cmds_names) {}

  auto get_args_map(std::span<char const *> args) {
    auto map = ArgsMap<Cmd const>();
    if (!args.empty()) {
      map.exec_path = std::string_view(args[0]);
      std::size_t current_positional_idx = 0;
      for (std::size_t index = 1; index < args.size();) {
        auto const cli_arg = std::string_view(args[index]);
        if (is_dash_dash(cli_arg)) {
          // +1 to ignore the dash-dash
          for (std::size_t offset = index + 1; offset < args.size();) {
            offset += assign_positional(map, args.subspan(offset), current_positional_idx, info);
            ++current_positional_idx;
          }
          index = args.size();
        } else if (looks_positional(cli_arg)) {
          // check if is command first
          if (auto const idx = find_cmd(this->cmd_ref.get().subcmds, cli_arg); idx != -1) {
            auto const parent_cmds_names = this->info.get_parent_cmds_names();
            auto const new_parent_cmds_names = fmt::format(
              "{: <{}}{}",
              parent_cmds_names,
              parent_cmds_names.size() + static_cast<int>(!parent_cmds_names.empty()),
              this->cmd_ref.get().name
            );
            int i = 0;
            auto const rest = args.subspan(index);
            // clang-format off
            std::apply(
              [&i, idx, &map, rest, &new_parent_cmds_names](auto&&... cmd) {
                (void)(( // cast to void to suppress unused warning
                i == idx
                  ? (map.subcmd =
                      CmdParser<typename std::remove_reference_t<decltype(cmd)>::type>(
                        cmd.get(), new_parent_cmds_names)(rest), true)
                  : (++i, false)
                ) || ...);
              },
              this->cmd_ref.get().subcmds
            );
            // clang-format on
            index += rest.size();
          } else {
            index += assign_positional(map, args.subspan(index), current_positional_idx, info);
            ++current_positional_idx;
          }
        } else if (auto const option = try_parse_option(cli_arg); option.has_value()) {
          index += assign_option(map, *option, args.subspan(index), info);
        } else if (auto const flag = get_if_long_flag(cli_arg); !flag.empty()) {
          index += assign_long_flag(map, flag, info);
        } else if (auto const flags = get_if_short_flags(cli_arg); !flags.empty()) {
          index += assign_short_flags(map, flags, info);
        } else {
          throw UnknownArgument(this->cmd_ref.get().name, cli_arg, this->info);
        }
      }
    }
    return map;
  }

  std::size_t assign_positional(
    ArgsMap<Cmd const> &map, std::span<char const *> args, std::size_t cur_pos_idx, CmdInfoGetter &info
  ) const {
    if (cur_pos_idx >= this->cmd_ref.get().amount_pos)
      throw UnexpectedPositional(this->cmd_ref.get().name, args[0], this->cmd_ref.get().amount_pos, this->info);

    try {
      std::size_t idx = 0;
      // clang-format off
      std::apply(
        [&idx, cur_pos_idx, &map, args, &info](auto&&... arg) {
          (void)(( // cast to void to suppress unused warning
          arg.type == ArgType::POS
            ? idx == cur_pos_idx
              ? (arg.action(map.args, arg, args[0], info), true)
              : (++idx, false)
            : false
          ) || ...);
        },
        this->cmd_ref.get().args
      );
      // clang-format on
      return 1;
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), this->info);
    }
  }

  std::optional<ParsedOption> try_parse_option(std::string_view const whole_arg) {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    bool const has_equals = eq_idx != std::string_view::npos;
    if (num_of_dashes == 1) {
      // short option, e.g. `-O`
      auto const name = whole_arg.substr(1, 1);
      auto const it = find_arg_if(this->cmd_ref.get().args, [name](auto const &a) {
        return a.type == ArgType::OPT && name == a.abbrev;
      });
      if (!it) return std::nullopt;

      if (has_equals) {
        if (whole_arg.length() > 3) {
          // has equals followed by some value, e.g. `-O=2`
          return ParsedOption{.arg = *it, .value = whole_arg.substr(3)};
        }
        // case left: things like `-O=`
        return ParsedOption{.arg = *it, .value = ""};
      }

      if (whole_arg.length() > 2 && std::isupper(name[0])) {
        // only followed by some value, e.g. `-O2`
        return ParsedOption{.arg = *it, .value = whole_arg.substr(2)};
      }

      // case left: has no value (next CLI argument could be it)
      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    if (num_of_dashes == 2 && whole_arg.length() > 3) {
      // long option, e.g. `--name`

      if (has_equals) {
        auto const name = whole_arg.substr(2, eq_idx - 2);
        auto const it = find_arg_if(this->cmd_ref.get().args, [name](auto const &a) {
          return a.type == ArgType::OPT && name == a.name;
        });
        if (!it) return std::nullopt;

        auto const value = whole_arg.substr(eq_idx + 1);
        return ParsedOption{.arg = *it, .value = value};
      }

      // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
      auto const name = whole_arg.substr(2);
      auto const it = find_arg_if(this->cmd_ref.get().args, [name](auto const &a) {
        return a.type == ArgType::OPT && name == a.name;
      });
      if (!it) return std::nullopt;

      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    // not an option
    return std::nullopt;
  }

  std::size_t assign_option(
    ArgsMap<Cmd const> &map, ParsedOption const &option, std::span<char const *> args, CmdInfoGetter &info
  ) const {
    std::size_t ret = 1;
    auto value = option.value.or_else([args, &ret]() -> std::optional<std::string_view> {
      if (args.size() > 1 && looks_positional(args[1])) {
        ret = 2;
        return args[1];
      }
      return std::nullopt;
    });

    try {
      std::size_t idx = 0;
      // clang-format off
      std::apply(
        [&idx,  &option, &map, &value, &info](auto&&... arg) {
          (void)(( // cast to void to suppress unused warning
          idx == option.arg.tuple_idx
            ? (arg.action(map.args, arg, value, info), true)
            : (++idx, false)
          ) || ...);
        },
        this->cmd_ref.get().args
      );
      // clang-format on
      return ret;
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), this->info);
    }
  }

  std::size_t assign_long_flag(ArgsMap<Cmd const> &map, std::string_view const flag, CmdInfoGetter &info) const {
    auto const it = find_arg_if(this->cmd_ref.get().args, [&flag](auto const &a) { return a.name == flag; });
    if (!it) throw UnknownArgument(this->cmd_ref.get().name, flag, this->info);
    if (it->type != ArgType::FLG) {
      throw WrongType(this->cmd_ref.get().name, flag, to_string(it->type), to_string(ArgType::FLG), this->info);
    }

    try {
      std::size_t idx = 0;
      // clang-format off
      std::apply(
        [&idx, &it, &map, &info](auto&&... arg) {
          (void)(( // cast to void to suppress unused warning
          idx == it->tuple_idx
            ? (arg.action(map.args, arg, std::nullopt, info), true)
            : (++idx, false)
          ) || ...);
        },
        this->cmd_ref.get().args
      );
      // clang-format on
      return 1;
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), this->info);
    }
  }

  std::size_t assign_short_flags(ArgsMap<Cmd const> &map, std::string_view const flags, CmdInfoGetter &info) const {
    for (std::size_t i = 0; i < flags.size(); ++i) {
      auto const flag = flags.substr(i, 1);
      auto const it = find_arg_if(this->cmd_ref.get().args, [&flag](auto const &a) { return a.abbrev == flag; });
      if (!it) throw UnknownArgument(this->cmd_ref.get().name, flag, this->info);
      if (it->type != ArgType::FLG) {
        throw WrongType(this->cmd_ref.get().name, flag, to_string(it->type), to_string(ArgType::FLG), this->info);
      }

      try {
        std::size_t idx = 0;
        // clang-format off
        std::apply(
          [&idx, &it, &map, &info](auto&&... arg) {
            (void)(( // cast to void to suppress unused warning
            idx == it->tuple_idx
              ? (arg.action(map.args, arg, std::nullopt, info), true)
              : (++idx, false)
            ) || ...);
          },
          this->cmd_ref.get().args
        );
        // clang-format on
      } catch (std::runtime_error const &e) {
        throw UserError(e.what(), this->info);
      }
    }

    return 1;
  }

  void check_contains_required(ArgsMap<Cmd const> const &map) const {
    std::vector<std::string_view> missing_arg_names;
    // clang-format off
    std::apply(
      [&map, &missing_arg_names](auto&&... arg) {
        (void)(( // cast to void to suppress unused warning
          arg.is_required && !map.has(arg.name)
            ? missing_arg_names.push_back(arg.name)
            : (void)0
        ), ...);
      },
      this->cmd_ref.get().args
    );
    // clang-format on

    if (!missing_arg_names.empty())
      throw MissingRequiredArguments(this->cmd_ref.get().name, missing_arg_names, this->info);
  }

  void set_defaults(ArgsMap<Cmd const> &map) const {
    // clang-format off
    std::apply(
      [&map](auto&&... arg) {
        (void)(( // cast to void to suppress unused warning
          arg.has_default() && !map.has(arg.name)
            ? (map.args[arg.name] = *arg.default_value, (void)0)
            : (void)0
        ), ...);
      },
      this->cmd_ref.get().args
    );
    // clang-format on
  }
};

// +---------------------+
// |   parsing helpers   |
// +---------------------+

constexpr bool is_dash_dash(std::string_view const whole_arg) noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

constexpr bool looks_positional(std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  return num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string_view::npos);
}

constexpr std::string_view get_if_short_flags(std::string_view const whole_arg) noexcept {
  auto const names = whole_arg.substr(1);
  auto const all_chars = std::ranges::all_of(names, [](char const c) { return std::isalpha(c); });
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 1 && names.length() >= 1 && all_chars) return names;
  return {};
}

constexpr std::string_view get_if_long_flag(std::string_view const whole_arg) noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2) return name;
  return {};
}

} // namespace opz

#endif // OPZIONI_PARSING_HPP
