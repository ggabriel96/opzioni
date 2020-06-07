#ifndef OPZIONI_H
#define OPZIONI_H

#include <any>
#include <memory>
#include <set>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "converters.hpp"
#include "exceptions.hpp"
#include "types.hpp"

namespace opz {

class ArgMap;
class ArgValue;

template <typename T> struct Arg {
  std::string name;
  std::string help_text;
  bool is_required = false;
  std::set<T> choices;
  std::optional<T> default_value = T{};
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
    if (this->default_value.has_value())
      this->without_default();
    return *this;
  }

  Arg<T> &not_required() noexcept {
    this->is_required = false;
    return *this;
  }

  Arg<T> &among(std::set<T> choices) {
    if (this->flag_value.has_value()) {
      throw InvalidArgument(fmt::format(
          "Trying to set `choices` for argument `{}` while it is set as flag. "
          "Please reconsider, as they are mutually-exclusive options",
          original_name()));
    }
    this->choices = choices;
    return *this;
  }

  Arg<T> &without_default() noexcept {
    this->default_value = std::nullopt;
    if (!this->is_required)
      this->required();
    return *this;
  }

  Arg<T> &with_default(T default_value) noexcept {
    this->default_value = default_value;
    if (this->is_required)
      this->not_required();
    return *this;
  }

  Arg<T> &as_flag(T flag_value = !T{}) {
    if (!this->choices.empty()) {
      throw InvalidArgument(fmt::format(
          "Trying to set argument `{}` as flag while also having `choices`. "
          "Please reconsider, as they are mutually-exclusive options",
          original_name()));
    }
    this->flag_value = flag_value;
    return *this;
  }

  Arg<T> &converted_by(AnyConverter converter) noexcept {
    this->converter = converter;
    return *this;
  }

  bool is_positional() const noexcept { return num_of_dashes == 0; }

  std::string original_name() const noexcept {
    if (this->is_positional())
      return this->name;
    else
      return fmt::format("{:->{}}", name, name.length() + num_of_dashes);
  }
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

/**
 * @TODO:
 * detect that required arguments are missing!
 * save exec name
 * program description
 * proram epilog
 *
 * allow unknown arguments?
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

  ArgMap parse(int, char const *[]) const;

private:
  std::vector<ArgInfo> positional_args;
  std::unordered_map<std::string, ArgInfo> options;

  ParseResult parse_args(int, char const *[]) const;
  ArgMap convert_args(ParseResult &&) const;

  template <typename T>
  void add_choice_checking_to_conversion(Arg<T> &spec) const noexcept {
    spec.converter = [arg_name = spec.name, choices = spec.choices,
                      converter = spec.converter](
                         std::optional<std::string> value) -> std::any {
      auto const result = std::any_cast<T>(converter(value));
      if (!choices.contains(result))
        throw InvalidChoice(fmt::format(
            "Argument `{}` cannot be set to `{}`. Allowed values are: [{}]",
            arg_name, result, fmt::join(choices, ", ")));
      return result;
    };
  }

  friend ArgMap;
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

  friend ArgMap ArgParser::convert_args(ParseResult &&) const;
};

} // namespace opz

#endif // OPZIONI_H
