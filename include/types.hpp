#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include "converters.hpp"
#include "memory.hpp"

#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace opzioni {

struct Arg {
  std::string name{};
  std::string description{};
  bool is_required = false;

  Arg &help(std::string) noexcept;
  Arg &required() noexcept;
};

struct ArgValue {
  std::string value{};

  template <typename T> T as() const { return convert<T>(value); }
};

struct ArgMap {
  ArgValue operator[](std::string name) const {
    if (!args.contains(name))
      throw UnknownArgument(fmt::format("Could not find argument `{}`", name));
    return args.at(name);
  }

  template <typename T> T get_if_else(T &&desired, std::string name, T &&otherwise) const {
    // mainly intended for flags which are not only true or false
    if (args.contains(name))
      return desired;
    else
      return otherwise;
  }

  template <typename T> T value_or(std::string name, T &&otherwise) const {
    if (auto const arg = args.find(name); arg != args.end())
      return arg->second.as<T>();
    else
      return otherwise;
  }

  template <typename T> T as(std::string name) const {
    auto const arg = (*this)[name];
    return arg.as<T>();
  }

  auto size() const noexcept { return this->args.size(); }

  std::string cmd_name;
  memory::ValuePtr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

struct ParseResult {
  std::string cmd_name;
  memory::ValuePtr<ParseResult> subcmd;
  std::vector<std::string> positional;
  std::map<std::string, std::string> options;
  std::set<std::string> flags;
};

struct ParsedOption {
  std::string name;
  std::optional<std::string> value;
};

class Program {
private:
  std::map<std::string, memory::ValuePtr<Program>> cmds;
  std::vector<Arg> positional_args;
  std::map<std::string, Arg> flags;
  std::map<std::string, Arg> options;

  bool is_flag(std::string const &) const noexcept;

  ArgMap assign_args(ParseResult const &) const;
  void assign_args_into(ArgMap &, ParseResult const &) const;

  void assign_positional_args(ArgMap &, std::vector<std::string> const &) const;
  void assign_flags(ArgMap &, std::set<std::string> const &) const;
  void assign_options(ArgMap &, std::map<std::string, std::string> const &) const;

public:
  std::string name{};
  std::string description{};
  std::string epilog{};

  Program() = default;
  Program(std::string name) : name(name) {}
  Program(std::string name, std::string description, std::string epilog)
      : name(name), description(description), epilog(epilog) {}

  Program &help(std::string) noexcept;
  Program &with_epilog(std::string) noexcept;

  Arg &pos(std::string);
  Arg &opt(std::string);
  Arg &flag(std::string);
  Program &cmd(std::string);
  ArgMap operator()(int, char const *[]) const;

  std::optional<std::string> is_positional(std::string const &) const noexcept;
  std::optional<std::string> is_long_flag(std::string const &) const noexcept;
  std::optional<std::string> is_short_flags(std::string const &) const noexcept;
  std::optional<ParsedOption> is_option(std::string const &) const noexcept;
  std::optional<decltype(cmds)::const_iterator> is_subcmd(std::string const &) const noexcept;
};

} // namespace opzioni

#endif // OPZIONI_TYPES_H
