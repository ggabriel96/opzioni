#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <algorithm>
#include <any>
#include <cctype>
#include <functional>
#include <map>
#include <print>
#include <span>
#include <string_view>
#include <vector>

#include "args_map.hpp"
#include "args_view.hpp"
#include "concepts.hpp"
#include "converters.hpp"
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
      std::optional<ArgView> ret = std::nullopt;

      // clang-format off
        (void) // cast to void to suppress unused warning
        (
          (p(elem) ? (ret = elem, true) : (false)) || ...
        );
      // clang-format on

      return ret;
    },
    haystack);
}

template <concepts::Command... Cmds>
int find_cmd(std::tuple<Cmds...> haystack, std::string_view name) {
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
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;

  std::reference_wrapper<Cmd const> cmd_ref;
  typename CommandParserOf<typename Cmd::subcmd_types>::type subparser{};

  explicit CommandParser(Cmd const &cmd) : cmd_ref(cmd) {}

  auto get_args_view(std::span<char const *> args) {
    ArgsView view;
    if (!args.empty()) {
      view.exec_path = std::string_view(args[0]);
      std::size_t current_positional_idx = 0;
      for (std::size_t index = 1; index < args.size();) {
        auto const arg = std::string_view(args[index]);
        if (is_dash_dash(arg)) {
          // +1 to ignore the dash-dash
          for (std::size_t offset = index + 1; offset < args.size();) {
            offset += assign_positional(view, args.subspan(offset), current_positional_idx);
            ++current_positional_idx;
          }
          index = args.size();
        } else if (looks_positional(arg)) {
          // check if is command first
          if (auto const idx = find_cmd(cmd_ref.get().subcmds, arg); idx != -1) {
            int i = 0;
            // clang-format off
            std::apply(
              [&i, idx, this](auto&&... cmd) {
                (void) // cast to void to suppress unused warning
                (
                  (
                    i == idx
                      ? (this->subparser.template emplace<CommandParser<std::remove_reference_t<decltype(cmd)>>>(cmd), true)
                      : (++i, false)
                  ) || ...
                );
              },
              cmd_ref.get().subcmds
            );
            auto const rest = args.subspan(index);
            view.subcmd = std::make_unique<ArgsView>(
              std::visit(overloaded{
                  [](std::monostate) { return ArgsView{}; },
                  [rest](auto &p) { return p.get_args_view(rest); }
                }, this->subparser));
            index += rest.size();
            // clang-format on
          } else {
            index += assign_positional(view, args.subspan(index), current_positional_idx);
            ++current_positional_idx;
          }
        } else if (auto const option = try_parse_option(arg); option.has_value()) {
          index += assign_option(view, *option, args.subspan(index));
        } else if (auto const flag = get_if_long_flag(arg); !flag.empty()) {
          index += assign_long_flag(view, flag);
        } else if (auto const flags = get_if_short_flags(arg); !flags.empty()) {
          index += assign_short_flags(view, flags);
        } else {
          throw UnknownArgument(arg);
        }
      }
    }
    return view;
  }

  std::size_t assign_positional(ArgsView &view, std::span<char const *> args, std::size_t cur_pos_idx) const {
    if (cur_pos_idx + 1 > cmd_ref.get().amount_pos) throw UnexpectedPositional(args[0], cmd_ref.get().amount_pos);
    view.positionals.emplace_back(args[0]);
    return 1;
  }

  std::optional<ParsedOption> try_parse_option(std::string_view const whole_arg) {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    bool const has_equals = eq_idx != std::string_view::npos;
    if (num_of_dashes == 1) {
      // short option, e.g. `-O`
      auto const name = whole_arg.substr(1, 1);
      auto const it =
        find_arg_if(cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.abbrev; });
      if (!it) return std::nullopt;
      if (it->type != ArgType::OPT) {
        throw WrongType(name, to_string(it->type), to_string(ArgType::OPT));
      }

      if (has_equals) {
        if (whole_arg.length() > 3) {
          // has equals followed by some value, e.g. `-O=2`
          return ParsedOption{.arg = *it, .value = whole_arg.substr(3)};
        }
        // TODO: should this `-O=` be handled like this?
        // return {name, ""};
        return std::nullopt; // tmply considered not an option
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
        auto const it =
          find_arg_if(cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
        if (!it) return std::nullopt;
        if (it->type != ArgType::OPT) {
          throw WrongType(name, to_string(it->type), to_string(ArgType::OPT));
        }

        auto const value = whole_arg.substr(eq_idx + 1);
        return ParsedOption{.arg = *it, .value = value};
      }

      // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
      auto const name = whole_arg.substr(2);
      auto const it =
        find_arg_if(cmd_ref.get().args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
      if (!it) return std::nullopt;
      if (it->type != ArgType::OPT) {
        throw WrongType(name, to_string(it->type), to_string(ArgType::OPT));
      }
      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    // not an option
    return std::nullopt;
  }

  std::size_t assign_option(ArgsView &view, ParsedOption const option, std::span<char const *> args) const {
    if (option.value) {
      // value lookup is by name, not abbrev
      view.options[option.arg.name] = *option.value;
      return 1;
    }

    if (args.size() > 1 && looks_positional(args[1])) {
      // value lookup is by name, not abbrev
      view.options[option.arg.name] = args[1];
      return 2;
    }

    if (option.arg.has_implicit) {
      // value lookup is by name, not abbrev
      view.options[option.arg.name] = std::nullopt; // will assign implicit value later
      return 1;
    }

    // report error using the name/abbrev the user used
    throw MissingValue(option.arg.name, 1, 0);
  }

  std::size_t assign_long_flag(ArgsView &view, std::string_view const flag) const {
    auto const it = find_arg_if(cmd_ref.get().args, [&flag](auto const &a) { return a.name == flag; });
    if (!it) {
      throw UnknownArgument(flag);
    }
    if (it->type != ArgType::FLG) {
      throw WrongType(flag, to_string(it->type), to_string(ArgType::FLG));
    }

    view.options[it->name] = std::nullopt;
    return 1;
  }

  std::size_t assign_short_flags(ArgsView &view, std::string_view const flags) const {
    for (std::size_t i = 0; i < flags.size(); ++i) {
      auto const flag = flags.substr(i, 1);
      auto const it = find_arg_if(cmd_ref.get().args, [&flag](auto const &a) { return a.abbrev == flag; });
      if (!it) {
        throw UnknownArgument(flag);
      }
      if (it->type != ArgType::FLG) {
        throw WrongType(flag, to_string(it->type), to_string(ArgType::FLG));
      }

      // value lookup is by name, not abbrev
      view.options[it->name] = std::nullopt;
    }
    return 1;
  }

  void check_contains_required(ArgsView const &view) const {
    std::vector<std::string_view> missing_arg_names;
    // clang-format off
    // check positionals first
    std::size_t visited_pos_count = 0;
    std::apply(
      [&view, &visited_pos_count, &missing_arg_names](auto&&... arg) {
        (void) // cast to void to suppress unused warning
        (
          (
            arg.type == ArgType::POS
            && arg.is_required
            && (++visited_pos_count > view.positionals.size())
              ? missing_arg_names.push_back(arg.name)
              : (void)0), ...
        );
      },
      cmd_ref.get().args
    );
    // then options and flags
    std::apply(
      [&view, &missing_arg_names](auto&&... arg) {
        (void) // cast to void to suppress unused warning
        (
          (
            arg.type != ArgType::POS
            && arg.is_required
            && !view.options.contains(arg.name)
              ? missing_arg_names.push_back(arg.name)
              : (void)0), ...
        );
      },
      cmd_ref.get().args
    );

    if (!missing_arg_names.empty())
      throw MissingRequiredArguments(view.exec_path, missing_arg_names);

    if (view.subcmd != nullptr && !std::holds_alternative<std::monostate>(this->subparser)) {
      std::visit(overloaded {
        [](std::monostate) {},
        [&view](auto const &p) { p.check_contains_required(*view.subcmd); }
      }, this->subparser);
    }
    // clang-format on
  }

  auto get_args_map(ArgsView const &view) const {
    auto map = ArgsMap<Cmd>{.exec_path = view.exec_path};
    std::size_t pos_count = 0;

    // clang-format off
    std::apply(
      [this, &map, &view, &pos_count](auto&&... arg) {
        (this->process(arg, map, view, pos_count), ...);
      },
      cmd_ref.get().args
    );

    if (view.subcmd != nullptr && !std::holds_alternative<std::monostate>(this->subparser)) {
      std::visit(overloaded {
        [](std::monostate) {},
        [&map, &view](auto const &p) { map.subcmd = p.get_args_map(*view.subcmd); }
      }, this->subparser);
    }
    // clang-format on

    return map;
  }

  template <typename T>
  auto process(Arg<T> const &arg, ArgsMap<Cmd> &map, ArgsView const &view, std::size_t &pos_count) const {
    switch (arg.type) {
      case ArgType::POS: {
        std::print("process POS {}, pos_count {}\n", arg.name, pos_count);
        if (pos_count < view.positionals.size()) {
          map.args[arg.name] = convert<T>(view.positionals[pos_count]);
          pos_count += 1;
        } else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        // check for arg being required is done in a later step
        break;
      }
      case ArgType::OPT: {
        std::print("process OPT {}\n", arg.name);
        auto const opt = view.options.find(arg.name);
        if (opt != view.options.end()) {
          std::print("OPT {} found, value: {}\n", arg.name, opt->second.value_or("<unset>"));
          // already checked if it has implicit value during parsing
          map.args[arg.name] = opt->second.has_value() ? convert<T>(*opt->second) : *arg.implicit_value;
        } else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        // check for arg being required is done in a later step
        break;
      }
      case ArgType::FLG: {
        std::print("process FLG {}\n", arg.name);
        if (view.options.contains(arg.name)) map.args[arg.name] = *arg.implicit_value;
        else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        break;
      }
    }
  }
};

template <concepts::Command Cmd>
auto parse(Cmd const &cmd, std::span<char const *> args) {
  auto parser = CommandParser(cmd);
  auto const view = parser.get_args_view(args);
  view.print_debug();
  // done separately in order to report all missing required args
  parser.check_contains_required(view);

  auto const map = parser.get_args_map(view);

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
