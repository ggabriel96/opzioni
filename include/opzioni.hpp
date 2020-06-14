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
  std::string name;
  std::string epilog;
  std::string description;

  ArgParser() = default;
  ArgParser(std::string name, std::string epilog, std::string description)
      : name(name), epilog(epilog), description(description) {}

  template <typename T> void add(Arg<T> spec) {
    if (!spec.choices.empty())
      add_choice_checking_to_conversion(spec);
    if (spec.is_positional())
      this->positional_args.emplace_back(spec);
    else
      this->options[spec.name] = ArgInfo(spec);
  }

  [[no_discard]] ArgParser *sub(Command const &command) {
    auto subp = subs.insert(subs.end(), make_unique<ArgParser>(command.name, command.epilog, command.description));
    return subp->get();
  }

  ArgMap parse(int, char const *[]) const;

private:
  std::vector<std::unique_ptr<ArgParser>> subs;
  std::vector<ArgInfo> positional_args;
  std::map<std::string, ArgInfo> options;

  [[no_discard]] decltype(auto) find_sub(std::string_view const) const noexcept;

  ParseResult parse_args(int, char const *[]) const;
  void parse_args_into(ParseResult *, int, char const *[], int = 1) const;
  std::unique_ptr<ParseResult> parse_args(int, char const *[], int) const;

  ArgMap convert_args(ParseResult &&) const;
  void convert_args_into(ArgMap *, ParseResult *) const;
  std::unique_ptr<ArgMap> convert_args(std::unique_ptr<ParseResult>) const;

  void assign_positional_args(ArgMap *, std::vector<std::string> const &) const;
  void assign_flags(ArgMap *, std::set<std::string> const &) const;
  void assign_options(ArgMap *, std::map<std::string, std::string> const &) const;

  template <typename T> void add_choice_checking_to_conversion(Arg<T> &spec) const noexcept {
    spec.converter = [arg_name = spec.name, choices = spec.choices,
                      converter = spec.converter](std::optional<std::string> value) -> std::any {
      auto const result = std::any_cast<T>(converter(value));
      if (!choices.contains(result))
        throw InvalidChoice(fmt::format("Argument `{}` cannot be set to `{}`. Allowed values are: [{}]", arg_name,
                                        result, fmt::join(choices, ", ")));
      return result;
    };
  }
};

} // namespace opz

#endif // OPZIONI_H
