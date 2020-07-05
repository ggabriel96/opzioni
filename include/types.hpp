#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "converters.hpp"

namespace opzioni {

struct Command {
  std::string name;
  std::string epilog;
  std::string description;
};

template <typename T> struct Arg {
  std::string name;
  std::string help_text;
  bool is_required = false;
  std::set<T> choices;
  std::optional<T> default_value = T{};
  std::optional<T> flag_value = std::nullopt;
  AnyConverter converter = opzioni::convert<T>;
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
      throw InvalidArgument(fmt::format("Trying to set `choices` for argument `{}` while it is set as flag. "
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
      throw InvalidArgument(fmt::format("Trying to set argument `{}` as flag while also having `choices`. "
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

struct ParseResult {
  std::string cmd_name;
  std::unique_ptr<ParseResult> subcmd;
  std::vector<std::string> positional;
  std::map<std::string, std::string> options;
  std::set<std::string> flags;
};

struct SplitArg {
  size_t num_of_dashes;
  std::string name;
  std::optional<std::string> value;
};

struct ArgValue {
  std::any value;

  template <typename TargetType> TargetType as() const { return std::any_cast<TargetType>(this->value); }
};

struct ArgMap {
  ArgValue const &operator[](std::string name) const {
    try {
      return args.at(name);
    } catch (std::out_of_range const &) {
      throw ArgumentNotFound(fmt::format("Could not find argument `{}`", name));
    }
  }

  auto size() const noexcept { return this->args.size(); }

  std::string cmd_name;
  std::unique_ptr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

} // namespace opzioni

#endif // OPZIONI_TYPES_H
