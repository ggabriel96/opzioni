#ifndef OPZIONI_H
#define OPZIONI_H

#include <memory>
#include <set>
#include <span>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "converters.hpp"
#include "exceptions.hpp"
#include "types.hpp"

namespace opzioni {

class Program {
public:
  std::string name;
  std::string epilog;
  std::string description;

  Program() = default;
  Program(std::string name, std::string epilog, std::string description)
      : name(name), epilog(epilog), description(description) {}

  void pos(Arg &&);
  void opt(Arg &&);
  void flag(Arg &&);
  [[no_discard]] Program *cmd(Command const &);
  [[no_discard]] ArgMap parse(int, char const *[]) const;

private:
  std::map<std::string, std::unique_ptr<Program>> cmds;
  std::vector<Arg> positional_args;
  std::map<std::string, Arg> flags;
  std::map<std::string, Arg> options;

  bool is_flag(std::string const &) const noexcept;
  bool arg_is_long_flag(std::string const &) const noexcept;
  bool arg_is_short_flags(std::string const &) const noexcept;

  ParseResult parse_args(std::span<char const *>) const;
  void parse_args_into(ParseResult &, std::span<char const *>) const;

  ArgMap assign_args(ParseResult const &) const;
  void assign_args_into(ArgMap &, ParseResult const &) const;

  void assign_positional_args(ArgMap &, std::vector<std::string> const &) const;
  void assign_flags(ArgMap &, std::set<std::string> const &) const;
  void assign_options(ArgMap &, std::map<std::string, std::string> const &) const;
};

} // namespace opzioni

#endif // OPZIONI_H
