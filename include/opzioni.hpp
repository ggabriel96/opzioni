#ifndef OPZIONI_H
#define OPZIONI_H

#include "args.hpp"
#include "memory.hpp"

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace opzioni {

class Program;

namespace parsing {

struct ParsedOption {
  std::string name;
  std::optional<std::string> value;
};

ParsedOption parse_option(std::string) noexcept;

struct Command {
  Program &program;
  std::size_t index;
};

struct DashDash {
  std::size_t index;
};

struct Flag {
  std::string name;
};

struct ManyFlags {
  std::string chars;
};

struct Option {
  ParsedOption arg;
  size_t index;
};

struct Positional {
  std::size_t index;
};

struct Unknown {
  std::size_t index;
};

using alternatives = std::variant<Command, DashDash, Flag, ManyFlags, Option, Positional, Unknown>;

class Parser {
public:
  Parser(Program const &spec, std::span<char const *> args) : spec(spec), args(args) {}

  ArgMap operator()();
  std::size_t operator()(parsing::Flag);
  std::size_t operator()(parsing::Option);
  std::size_t operator()(parsing::Command);
  std::size_t operator()(parsing::DashDash);
  std::size_t operator()(parsing::ManyFlags);
  std::size_t operator()(parsing::Positional);
  std::size_t operator()(parsing::Unknown);

private:
  ArgMap map;
  Program const &spec;
  std::span<char const *> args;
  std::size_t current_positional_idx{};

  parsing::alternatives decide_type(std::size_t) const noexcept;

  bool is_dash_dash(std::string const &) const noexcept;
  std::optional<std::string> looks_positional(std::string const &) const noexcept;

  bool would_be_positional(std::size_t) const noexcept;
};

} // namespace parsing

struct Program {
  std::string name{};
  std::string description{};
  std::string epilog{};

  std::map<std::string, Flag> flags;
  std::vector<Positional> positionals;
  std::map<std::string, Option> options;
  std::map<std::string, memory::ValuePtr<Program>> cmds;

  Program() = default;
  Program(std::string name) : name(name) {}
  Program(std::string name, std::string description, std::string epilog)
      : name(name), description(description), epilog(epilog) {}

  Program &help(std::string) noexcept;
  Program &with_epilog(std::string) noexcept;

  Flag &flag(std::string);
  Option &opt(std::string);
  Program &cmd(std::string);
  Positional &pos(std::string);

  ArgMap operator()(int, char const *[]);
  ArgMap operator()(std::span<char const *>);

  void set_defaults(ArgMap &) const noexcept;

  Program *is_command(std::string const &) const noexcept;
  bool is_flag(std::string const &) const noexcept;
  std::optional<std::string> is_long_flag(std::string const &) const noexcept;
  std::optional<std::string> is_short_flags(std::string const &) const noexcept;
  std::optional<parsing::ParsedOption> is_option(std::string const &) const noexcept;
};

} // namespace opzioni

#endif // OPZIONI_H
