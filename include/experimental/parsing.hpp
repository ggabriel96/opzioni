#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <algorithm>
#include <any>
#include <cctype>
#include <map>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "converters.hpp"
#include "exceptions.hpp"
#include "experimental/fixed_string.hpp"
#include "experimental/program.hpp"
#include "experimental/string_list.hpp"
#include "experimental/type_list.hpp"

// +---------------------+
// |      fwd decls      |
// +---------------------+

struct ParsedOption {
  std::string_view name;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;

  bool is_valid() const noexcept {
    return !name.empty();
  }
};

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view get_if_short_flags(std::string_view const) noexcept;
constexpr std::string_view get_if_long_flag(std::string_view const) noexcept;
constexpr ParsedOption try_parse_option(std::string_view const) noexcept;

// +-----------------------+
// |   main parsing code   |
// +-----------------------+

struct ArgsView {
  std::string_view exec_path{};
  // TODO: put positionals and options on the same map when we
  // start querying the program args?
  std::vector<std::string_view> positionals;
  std::map<std::string_view, std::string_view> options;
};

template <typename...>
struct ArgsMap;

template <fixed_string... ArgNames, typename... ArgTypes>
struct ArgsMap<StringList<ArgNames...>, TypeList<ArgTypes...>> {
  using arg_names = StringList<ArgNames...>;
  using arg_types = TypeList<ArgTypes...>;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;

  template<fixed_string Name>
  typename GetType<Name, arg_names, arg_types>::type get() const {
    using T = GetType<Name, arg_names, arg_types>::type;
    static_assert(!std::is_same_v< T, void >, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw opzioni::ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  // TODO: get_or(<default value>)

  bool has(std::string_view name) const noexcept { return args.contains(name); }

  auto size() const noexcept { return args.size(); }
};

template <typename...>
struct ArgParser;

template <fixed_string... ArgNames, typename... ArgTypes>
struct ArgParser<StringList<ArgNames...>, TypeList<ArgTypes...>> {

  Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program;

  ArgParser(Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program) : program(program) {}

  // auto operator()(std::span<char const *> args) {
  //   ArgsMap map;
  //   map.exec_path = args[0];
  //   return map;
  // }

  auto get_args_view(std::span<char const *> args) {
    ArgsView view;
    if (args.size() > 0) {
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
        // } else if (auto cmd = is_command(program, arg); cmd != nullptr) {
        //   index += assign_command(map, args.subspan(index), *cmd);
        } else if (looks_positional(arg)) {
          index += assign_positional(view, args.subspan(index), current_positional_idx);
          ++current_positional_idx;
        } else if (auto const option = try_parse_option(arg); !option.name.empty()) {
          index += assign_option(view, option, args.subspan(index));
        } else if (auto const flag = get_if_long_flag(arg); !flag.empty()) {
          index += assign_long_flag(view, flag);
        } else if (auto const flags = get_if_short_flags(arg); !flags.empty()) {
          index += assign_short_flags(view, flags);
        } else {
          throw opzioni::UnknownArgument(arg);
        }
      }
    }
    return view;
  }

  std::size_t assign_positional(ArgsView &view, std::span<char const *> args, std::size_t cur_pos_idx) const {
    if (cur_pos_idx + 1 > program.amount_pos)
      throw opzioni::UnexpectedPositional(args[0], program.amount_pos);
    view.positionals.emplace_back(args[0]);
    return 1;
  }

  std::size_t assign_option(ArgsView &view, ParsedOption const option, std::span<char const *> args) const {
    if (option.value) {
      view.options[option.name] = *option.value;
      return 1;
    }

    auto const it = std::ranges::find_if(program.args, [&option](auto const &a) { return a.name == option.name; });
    if (it == program.args.end()) {
      throw opzioni::UnknownArgument(option.name);
    }
    if (it->type != ArgType::OPT) {
      throw opzioni::WrongType(option.name, ToString(it->type), ToString(ArgType::OPT));
    }

    if (args.size() > 1 && looks_positional(args[1])) {
      view.options[option.name] = args[1];
      return 2;
    }

    throw opzioni::MissingValue(option.name, 1, 0);
  }

  std::size_t assign_long_flag(ArgsView &view, std::string_view const flag) const {
    auto const it = std::ranges::find_if(program.args, [&flag](auto const &a) { return a.name == flag; });
    if (it == program.args.end()) {
      throw opzioni::UnknownArgument(flag);
    }
    if (it->type != ArgType::FLG) {
      throw opzioni::WrongType(flag, ToString(it->type), ToString(ArgType::FLG));
    }

    view.options[it->name] = "";
    return 1;
  }

  std::size_t assign_short_flags(ArgsView &view, std::string_view const flags) const {
    for (std::size_t i = 0; i < flags.size(); ++i) {
      auto const flag = flags.substr(i, 1);
      auto const it = std::ranges::find_if(program.args, [&flag](auto const &a) { return a.abbrev == flag; });
      if (it == program.args.end()) {
        throw opzioni::UnknownArgument(flag);
      }
      if (it->type != ArgType::FLG) {
        throw opzioni::WrongType(flag, ToString(it->type), ToString(ArgType::FLG));
      }

      // value lookup is by name, not abbrev
      view.options[it->name] = "";
    }
    return 1;
  }

  auto get_args_map(ArgsView const &view) const {
    auto map = ArgsMap<StringList<ArgNames...>, TypeList<ArgTypes...>>{.exec_path = view.exec_path};
    std::size_t idx = sizeof...(ArgTypes) - 1;
    std::size_t idx_pos = 0;
    ((process<ArgNames>(idx, map, view, idx_pos), idx--), ...);
    return map;
  }

  template<fixed_string ArgName>
  auto process(std::size_t idx, ArgsMap<StringList<ArgNames...>, TypeList<ArgTypes...>> &map, ArgsView const &view, std::size_t &idx_pos) const {
    using T = GetType<ArgName, StringList<ArgNames...>, TypeList<ArgTypes...>>::type;
    auto const arg = program.args[idx];
    switch (arg.type) {
      case ArgType::POS: {
        std::print("process POS {}=={}, idx {}, idx_pos {}\n", ArgName.data, arg.name, idx, idx_pos);
        if (idx_pos < view.positionals.size()) {
          map.args[ArgName] = opzioni::convert<T>(view.positionals[idx_pos]);
          idx_pos += 1;
        } else if (arg.is_required) {
          // TODO
          throw "missing positional";
        }
        break;
      }
      case ArgType::OPT: {
        std::print("process OPT {}=={}, idx {}\n", ArgName.data, arg.name, idx);
        auto const opt = view.options.find(ArgName);
        if (opt != view.options.end()) {
          std::print("OPT {} found, value: {}\n", ArgName.data, opt->second);
          map.args[ArgName] = opzioni::convert<T>(opt->second);
        } else if (arg.is_required) {
          // TODO
          throw "missing option";
        }
        break;
      }
      case ArgType::FLG: {
        std::print("process FLG {}=={}, idx {}\n", ArgName.data, arg.name, idx);
        map.args[ArgName] = view.options.contains(ArgName);
        break;
      }
    }
  }
};

template <fixed_string... ArgNames, typename... ArgTypes>
auto parse(Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program, std::span<char const *> args) {
  auto parser  = ArgParser<StringList<ArgNames...>, TypeList<ArgTypes...>>(program);
  auto const view = parser.get_args_view(args);
  std::print("positionals ({}):", view.positionals.size());
  for (auto &&p : view.positionals) {
    std::print(" {}", p);
  }
  std::print("\n");

  std::print("options & flags:\n");
  for (auto &&o : view.options) {
    std::print("- {} = {}\n", o.first, o.second);
  }

  auto const map = parser.get_args_map(view);

  // auto map = parse_args(program, args);
  // check_contains_required(program, map);
  return map;
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
  if (num_of_dashes == 1 && names.length() >= 1 && all_chars)
    return names;
  return {};
}

constexpr std::string_view get_if_long_flag(std::string_view const whole_arg) noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2)
    return name;
  return {};
}

constexpr ParsedOption try_parse_option(std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const eq_idx = whole_arg.find('=', num_of_dashes);
  bool const has_equals = eq_idx != std::string_view::npos;
  if (num_of_dashes == 1) {
    // short option, e.g. `-O`
    auto const name = whole_arg.substr(1, 1);
    if (has_equals) {
      if (whole_arg.length() > 3) {
        // has equals followed by some value, e.g. `-O=2`
        return {name, whole_arg.substr(3)};
      }
      // TODO: should this `-O=` be handled like this?
      // return {name, ""};
      return {"", std::nullopt}; // tmply considered not an option
    }

    if (whole_arg.length() > 2 && std::isupper(name[0])) {
      // only followed by some value, e.g. `-O2`
      return {name, whole_arg.substr(2)};
    }

    // TODO: case left: has no value (next CLI argument could be it)
    // return {name, std::nullopt};
    return {"", std::nullopt}; // tmply considered not an option
  }

  if (num_of_dashes == 2 && whole_arg.length() > 3) {
    // long option, e.g. `--name`
    if (has_equals) {
      auto const name = whole_arg.substr(2, eq_idx - 2);
      auto const value = whole_arg.substr(eq_idx + 1);
      return {name, value};
    }

    // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
    return {whole_arg.substr(2), std::nullopt};
  }

  // not an option
  return {"", std::nullopt};
}

#endif