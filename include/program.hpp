#ifndef OPZIONI_PROGRAM_H
#define OPZIONI_PROGRAM_H

#include "args.hpp"
#include "memory.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace opzioni {

struct ParsedOption {
  std::string name;
  std::optional<std::string> value;
};

ParsedOption parse_option(std::string const &) noexcept;

struct Program {
  std::string name{};
  std::string description{};
  std::string epilog{};

  std::map<std::string, memory::ValuePtr<Program>> cmds;
  std::vector<Positional> positionals;
  std::map<std::string, Flag> flags;
  std::map<std::string, Option> options;

  Program() = default;
  Program(std::string name) : name(name) {}
  Program(std::string name, std::string description, std::string epilog)
      : name(name), description(description), epilog(epilog) {}

  Program &help(std::string) noexcept;
  Program &with_epilog(std::string) noexcept;

  ArgMap operator()(int, char const *[]) const;

  Positional &pos(std::string);
  Option &opt(std::string);
  Flag &flag(std::string);
  Program &cmd(std::string);

  bool is_flag(std::string const &) const noexcept;
  void set_defaults(ArgMap &) const noexcept;

  std::optional<std::string> is_positional(std::string const &) const noexcept;
  std::optional<std::string> is_long_flag(std::string const &) const noexcept;
  std::optional<std::string> is_short_flags(std::string const &) const noexcept;
  std::optional<ParsedOption> is_option(std::string const &) const noexcept;
  std::optional<decltype(cmds)::const_iterator> is_subcmd(std::string const &) const noexcept;
};

} // namespace opzioni

#endif // OPZIONI_PROGRAM_H
