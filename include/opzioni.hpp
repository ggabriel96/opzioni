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
  void assign_positional_args(ArgMap &, std::vector<std::string> const &) const;
  void assign_flags(ArgMap &, std::unordered_set<std::string> const &) const;
  void
  assign_options(ArgMap &,
                 std::unordered_map<std::string, std::string> const &) const;

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
};

} // namespace opz

#endif // OPZIONI_H
