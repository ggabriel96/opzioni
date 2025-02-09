#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <any>
#include <map>
#include <print>
#include <span>
#include <string>
#include <string_view>
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
  std::vector<std::string_view> positionals;
  std::map<std::string_view, std::string_view> options;
};

struct ArgMap {
  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;

  std::any operator[](std::string_view name) const {
    if (!args.contains(name)) throw opzioni::ArgumentNotFound(name);
    return args.at(name);
  }

  template <typename T>
  T as(std::string_view name) const {
    auto const arg = (*this)[name];
    return std::any_cast<T>(arg);
  }

  bool has(std::string_view name) const noexcept { return args.contains(name); }

  auto size() const noexcept { return this->args.size(); }
};

template <typename...>
struct ArgParser;

template <fixed_string... ArgNames, typename... ArgTypes>
struct ArgParser<StringList<ArgNames...>, TypeList<ArgTypes...>> {

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;
  Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program;

  ArgParser(Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program) : program(program) {}

  ArgMap operator()(std::span<char const *> args) {
    ArgMap map;
    map.exec_path = args[0];
    return map;
  }

  auto get_args_view(std::span<char const *> args) {
    ArgsView view;
    if (args.size() > 0) {
      std::size_t current_positional_idx = 0;
      for (std::size_t index = 1; index < args.size();) {
        auto const arg = std::string_view(args[index]);
        if (is_dash_dash(arg)) {
          // +1 to ignore the dash-dash
          auto const rest = args.subspan(index + 1);
          view.positionals.reserve(view.positionals.size() + rest.size());
          for (auto const &str : rest) {
            view.positionals.emplace_back(str);
          }
          current_positional_idx += rest.size();
          index = args.size();
        // } else if (auto cmd = is_command(program, arg); cmd != nullptr) {
        //   index += assign_command(map, args.subspan(index), *cmd);
        } else if (looks_positional(arg)) {
          view.positionals.emplace_back(arg);
          ++current_positional_idx;
          ++index;
        } else if (auto const option = try_parse_option(arg); option.value.has_value()) {
          view.options[option.name] = *option.value;
          index += 1; // TODO: tmply only accepting options with their values "glued" together
        // } else if (auto const flags = is_short_flags(program, arg); !flags.empty()) {
        //   index += assign_many_flags(program, map, flags);
        // } else if (auto const flag = is_long_flag(program, arg); !flag.empty()) {
        //   index += assign_flag(program, map, flag);
        } else {
          throw opzioni::UnknownArgument(arg);
        }
      }
    }
    return view;
  }
};

template <fixed_string... ArgNames, typename... ArgTypes>
auto parse(Program<StringList<ArgNames...>, TypeList<ArgTypes...>> const &program, std::span<char const *> args) {
  auto result  = ArgParser<StringList<ArgNames...>, TypeList<ArgTypes...>>(program);
  auto const i = result.get_args_view(args);
  std::print("positionals ({}):", i.positionals.size());
  for (auto &&p : i.positionals) {
    std::print(" {}", p);
  }
  std::print("\n");

  std::print("options & flags:\n");
  for (auto &&o : i.options) {
    std::print("- {} = {}\n", o.first, o.second);
  }

  // auto map = parse_args(program, args);
  // check_contains_required(program, map);
  return result;
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
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  if (num_of_dashes == 1 && flags.length() >= 1)
    return flags;
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

    if (whole_arg.length() > 2) {
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
    // TODO: return {whole_arg.substr(2), std::nullopt};
    return {"", std::nullopt}; // tmply considered not an option
  }

  // not an option
  return {"", std::nullopt};
}

#endif