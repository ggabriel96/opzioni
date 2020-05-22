#ifndef OPZIONI_H
#define OPZIONI_H

#include <any>
#include <charconv>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "exceptions.hpp"

namespace opz {

class ArgMap;
class ArgValue;

using AnyConverter = std::function<std::any(std::optional<std::string>)>;

template <typename TargetType>
using TypedConverter = std::function<TargetType(std::optional<std::string>)>;

template <typename TargetType>
auto convert(std::optional<std::string>) -> TargetType;

template <typename T> struct Arg {
  std::string name;
  std::string help_text;
  bool is_required = false;
  std::set<T> choices;
  std::optional<T> default_value = std::nullopt;
  std::optional<T> flag_value = std::nullopt;
  AnyConverter converter = opz::convert<T>;
  decltype(std::string::npos) num_of_dashes;

  Arg(std::string name) noexcept : num_of_dashes{name.find_first_not_of('-')} {
    if (is_positional())
      this->name = name;
    else
      this->name = name.substr(num_of_dashes);
  }

  Arg<T> &help(std::string help_text) noexcept {
    this->help_text = help_text;
    return *this;
  }

  Arg<T> &required() noexcept {
    this->is_required = true;
    return *this;
  }

  Arg<T> &not_required() noexcept {
    this->is_required = false;
    return *this;
  }

  Arg<T> &among(std::set<T> choices) noexcept {
    this->choices = choices;
    return *this;
  }

  Arg<T> &with_default(T default_value) noexcept {
    this->default_value = default_value;
    return *this;
  }

  Arg<bool> &as_flag() noexcept {
    this->flag_value = true;
    return with_default(false);
  }

  Arg<T> &as_flag(T flag_value) noexcept {
    this->flag_value = flag_value;
    return *this;
  }

  Arg<T> &converted_by(AnyConverter converter) noexcept {
    this->converter = converter;
    return *this;
  }

  bool is_positional() const noexcept { return num_of_dashes == 0; }
};

struct ArgInfo {
  std::string name;
  std::string help;
  bool required = false;
  std::any default_value;
  std::any flag_value;
  AnyConverter converter;

  ArgInfo() = default;

  template <typename T> explicit ArgInfo(Arg<T> const &arg) noexcept {
    name = arg.name;
    help = arg.help_text;
    required = arg.is_required;
    default_value = arg.default_value ? *arg.default_value : std::any{};
    flag_value = arg.flag_value ? *arg.flag_value : std::any{};
    converter = arg.converter;
  }

  bool is_flag() const { return this->flag_value.has_value(); }
};

struct SplitArg {
  size_t num_of_dashes;
  std::string name;
  std::optional<std::string> value;
};

template <typename TargetType>
TargetType apply_conversion(AnyConverter const &convert_fn,
                            std::optional<std::string> const &str_value) {
  return std::any_cast<TargetType>(convert_fn(str_value));
}

template <typename TargetType>
TargetType apply_conversion(TypedConverter<TargetType> const &convert_fn,
                            std::optional<std::string> const &str_value) {
  return convert_fn(str_value);
}

/**
 * exec name
 * description
 * epilog
 *
 * allow unknown arguments
 */
class ArgParser {
public:
  template <typename T> void add(Arg<T> spec) {
    if (!spec.choices.empty())
      add_choice_checking_to_conversion(spec);
    if (spec.is_positional())
      this->positional_args.emplace_back(spec);
    else
      this->options[spec.name] = ArgInfo(spec);
  }

  ArgMap parse_args(int, char const *[]) const;

private:
  std::vector<ArgInfo> positional_args;
  std::unordered_map<std::string, ArgInfo> options;

  bool is_multiple_short_flags(std::string const &) const;

  template <typename T>
  void add_choice_checking_to_conversion(Arg<T> &spec) const noexcept {
    spec.converter = [arg_name = spec.name, choices = spec.choices,
                      converter = spec.converter](
                         std::optional<std::string> value) -> std::any {
      auto const result = apply_conversion<T>(converter, value);
      if (!choices.contains(result))
        throw InvalidChoice(fmt::format(
            "Argument `{}` cannot be set to `{}`. Allowed values are: [{}]",
            arg_name, result, fmt::join(choices, ", ")));
      return result;
    };
  }
};

struct ArgValue {
  std::any value;

  template <typename TargetType> TargetType as() const {
    return std::any_cast<TargetType>(this->value);
  }
};

class ArgMap {
public:
  ArgValue const &operator[](std::string arg_name) const {
    if (!args.contains(arg_name))
      throw ArgumentNotFound(
          fmt::format("Could not find argument `{}`", arg_name));
    return args.at(arg_name);
  }

  auto size() const noexcept { return this->args.size(); }

  std::unique_ptr<std::vector<std::string>> remaining_args;

private:
  std::unordered_map<std::string, ArgValue> args;

  void set_defaults_for_missing_options(
      std::unordered_map<std::string, ArgInfo> const &);

  friend ArgMap ArgParser::parse_args(int, char const *[]) const;
};

} // namespace opz

#endif // OPZIONI_H
