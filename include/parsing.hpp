#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <algorithm>
#include <cctype>
#include <map>
#include <print>
#include <span>
#include <string_view>
#include <vector>

#include "actions.hpp"
#include "args_map.hpp"
#include "concepts.hpp"
#include "exceptions.hpp"
#include "variant.hpp"

namespace opz {

// +---------------------+
// |      fwd decls      |
// +---------------------+

struct ParsedOption {
  ArgView arg;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view get_if_short_flags(std::string_view const) noexcept;
constexpr std::string_view get_if_long_flag(std::string_view const) noexcept;

template <typename... Ts>
auto find_arg_if(std::tuple<Arg<Ts>...> haystack, std::predicate<ArgView> auto p) {
  return std::apply(
    [&p](auto &&...elem) {
      std::size_t idx = 0;
      std::optional<ArgView> ret = std::nullopt;
      // clang-format off
        (void) // cast to void to suppress unused warning
        ((p(elem) ? (ret = ArgView(idx, elem), true) : (++idx, false)) || ...);
      // clang-format on
      return ret;
    },
    haystack);
}

template <concepts::Command... Cmds>
int find_cmd(std::tuple<Cmds...> haystack, std::string_view name) { // TODO: add const to Cmds...?
  return std::apply(
    [name](auto &&...elem) {
      int idx = 0, ret = -1;
      (void) // cast to void to suppress unused warning
      ((elem.name == name ? (ret = idx, true) : (++idx, false)) || ...);
      return ret;
    },
    haystack);
}

// +-----------------------+
// |   main parsing code   |
// +-----------------------+

template <concepts::Command>
struct CommandParser;

template <typename...>
struct CommandParserOf;
template <concepts::Command... Cmds>
struct CommandParserOf<TypeList<Cmds...>> {
  using type = std::variant<std::monostate, CommandParser<Cmds const>...>;
};

template <concepts::Command Cmd>
struct CommandParser {
  using cmd_type = Cmd;
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;

  std::reference_wrapper<Cmd const> cmd_ref;
  typename CommandParserOf<typename Cmd::subcmd_types>::type subparser{};

  explicit CommandParser(Cmd const &cmd) : cmd_ref(cmd) {}

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
            offset += assign_positional(map, args.subspan(offset), current_positional_idx);
            ++current_positional_idx;
          }
          index = args.size();
        } else if (looks_positional(cli_arg)) {
          // check if is command first
          if (auto const idx = find_cmd(this->cmd_ref.get().subcmds, cli_arg); idx != -1) {
            int i = 0;
            // clang-format off
            std::apply(
              [&i, idx, this](auto&&... cmd) {
                (void)(( // cast to void to suppress unused warning
                i == idx
                  ? (this->subparser.template emplace<CommandParser<std::remove_reference_t<decltype(cmd)>>>(cmd), true)
                  : (++i, false)
                ) || ...);
              },
              this->cmd_ref.get().subcmds
            );
            auto const rest = args.subspan(index);
            std::visit(overloaded {
              [](std::monostate) {},
              [&map, rest](auto &p) { map.subcmd = p.get_args_map(rest); }
            }, this->subparser);
            index += rest.size();
            // clang-format on
          } else {
            index += assign_positional(map, args.subspan(index), current_positional_idx);
            ++current_positional_idx;
          }
        } else if (auto const option = try_parse_option(cli_arg, map.exec_path); option.has_value()) {
          index += assign_option(map, *option, args.subspan(index));
        } else if (auto const flag = get_if_long_flag(cli_arg); !flag.empty()) {
          index += assign_long_flag(map, flag);
        } else if (auto const flags = get_if_short_flags(cli_arg); !flags.empty()) {
          index += assign_short_flags(map, flags);
        } else {
          throw UnknownArgument(map.exec_path, cli_arg);
        }
      }
    }
    return map;
  }

  std::size_t assign_positional(ArgsMap<Cmd const> &map, std::span<char const *> args, std::size_t cur_pos_idx) const {
    if (cur_pos_idx >= this->cmd_ref.get().amount_pos)
      throw UnexpectedPositional(map.exec_path, args[0], this->cmd_ref.get().amount_pos);

    // clang-format off
    std::size_t idx = 0;
    std::apply(
      [this, &map, args, cur_pos_idx, &idx](auto&&... arg) {
        (void)(( // cast to void to suppress unused warning
        arg.type == ArgType::POS
          ? idx == cur_pos_idx
            ? (apply_action(this->cmd_ref.get(), map, arg, args[0]), true)
            : (++idx, false)
          : false
        ) || ...);
      },
      this->cmd_ref.get().args
    );
    // clang-format on
    return 1;
  }

  std::optional<ParsedOption> try_parse_option(std::string_view const whole_arg, std::string_view const cmd_name) {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    bool const has_equals = eq_idx != std::string_view::npos;
    if (num_of_dashes == 1) {
      // short option, e.g. `-O`
      auto const name = whole_arg.substr(1, 1);
      auto const it = find_arg_if(
        this->cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.abbrev; });
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
        auto const it = find_arg_if(
          this->cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
        if (!it) return std::nullopt;

        auto const value = whole_arg.substr(eq_idx + 1);
        return ParsedOption{.arg = *it, .value = value};
      }

      // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
      auto const name = whole_arg.substr(2);
      auto const it = find_arg_if(
        this->cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
      if (!it) return std::nullopt;

      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    // not an option
    return std::nullopt;
  }

  std::size_t assign_option(ArgsMap<Cmd const> &map, ParsedOption const &option, std::span<char const *> args) const {
    std::size_t ret = 1;
    auto value = option.value
      .or_else([args, &ret]() -> std::optional<std::string_view> {
        if (args.size() > 1 && looks_positional(args[1])) {
          ret = 2;
          return args[1];
        }
        return std::nullopt;
      });

    if (!value.has_value() && !option.arg.has_implicit)
      throw MissingValue(map.exec_path, option.arg.name, 1, 0);

    // clang-format off
    std::size_t idx = 0;
    std::apply(
      [this, &map, &option, &value, &idx](auto&&... arg) {
        (void)(( // cast to void to suppress unused warning
        idx == option.arg.tuple_idx
          ? (value.has_value() || arg.has_implicit())
            ? (apply_action(this->cmd_ref.get(), map, arg, value), true)
            : (false)
          : (++idx, false)
        ) || ...);
      },
      this->cmd_ref.get().args
    );
    // clang-format on

    return ret;
  }

  std::size_t assign_long_flag(ArgsMap<Cmd const> &map, std::string_view const flag) const {
    auto const it = find_arg_if(this->cmd_ref.get().args, [&flag](auto const &a) { return a.name == flag; });
    if (!it) throw UnknownArgument(map.exec_path, flag);
    if (it->type != ArgType::FLG) {
      throw WrongType(map.exec_path, flag, to_string(it->type), to_string(ArgType::FLG));
    }

    // clang-format off
    std::size_t idx = 0;
    std::apply(
      [this, &map, &it, &idx](auto&&... arg) {
        (void)(( // cast to void to suppress unused warning
        idx == it->tuple_idx
          ? (apply_action(this->cmd_ref.get(), map, arg, std::nullopt), true)
          : (++idx, false)
        ) || ...);
      },
      this->cmd_ref.get().args
    );
    // clang-format on

    return 1;
  }

  std::size_t assign_short_flags(ArgsMap<Cmd const> &map, std::string_view const flags) const {
    for (std::size_t i = 0; i < flags.size(); ++i) {
      auto const flag = flags.substr(i, 1);
      auto const it = find_arg_if(this->cmd_ref.get().args, [&flag](auto const &a) { return a.abbrev == flag; });
      if (!it) throw UnknownArgument(map.exec_path, flag);
      if (it->type != ArgType::FLG) {
        throw WrongType(map.exec_path, flag, to_string(it->type), to_string(ArgType::FLG));
      }

      // clang-format off
      std::size_t idx = 0;
      std::apply(
        [this, &map, &it, &idx](auto&&... arg) {
          (void)(( // cast to void to suppress unused warning
          idx == it->tuple_idx
            ? (apply_action(this->cmd_ref.get(), map, arg, std::nullopt), true)
            : (++idx, false)
          ) || ...);
        },
        this->cmd_ref.get().args
      );
      // clang-format on
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

    if (!missing_arg_names.empty())
      throw MissingRequiredArguments(map.exec_path, missing_arg_names);

    if (map.has_subcmd()) {
      std::visit(overloaded {
        [](std::monostate) {},
        [&map](auto const &p) { p.check_contains_required(std::get<ArgsMap<typename std::remove_reference_t<decltype(p)>::cmd_type>>(map.subcmd)); }
      }, this->subparser);
    }
    // clang-format on
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

    if (map.has_subcmd()) {
      std::visit(overloaded {
        [](std::monostate) {},
        [&map](auto &p) { p.set_defaults(std::get<ArgsMap<typename std::remove_reference_t<decltype(p)>::cmd_type>>(map.subcmd)); }
      }, this->subparser);
    }
    // clang-format on
  }
};

template <concepts::Command Cmd>
auto parse(Cmd const &cmd, std::span<char const *> args) {
  auto parser = CommandParser(cmd);
  auto map = parser.get_args_map(args);

  // 1) check_contains_required done separately in order to report all missing required args;
  // 2) also done after get_args_map in order to allow things such as `--help` to work
  // without actually requiring the required arguments
  parser.check_contains_required(map);
  parser.set_defaults(map);

  return map;
}

template <concepts::Command Cmd>
auto parse(Cmd const &cmd, int argc, char const *argv[]) {
  auto const args = std::span{argv, static_cast<std::size_t>(argc)};
  return parse(cmd, args);
}

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

#endif // OPZIONI_PARSING_H
