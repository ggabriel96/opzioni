#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include "args.hpp"
#include "memory.hpp"
#include "program.hpp"

#include <map>
#include <span>
#include <string>
#include <variant>

namespace opzioni {

namespace parsing {

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
  std::string value;
};

struct Subcommand {
  std::map<std::string, memory::ValuePtr<Program>>::const_iterator cmd;
  std::size_t index;
};

struct Unknown {
  std::string arg;
};

using alternatives = std::variant<DashDash, Flag, ManyFlags, Option, Positional, Subcommand, Unknown>;

class ArgumentParser {
public:
  ArgumentParser(Program const &program, std::span<char const *> args) : spec(program), args(args) {}

  ArgMap operator()();
  std::size_t operator()(DashDash);
  std::size_t operator()(Flag);
  std::size_t operator()(ManyFlags);
  std::size_t operator()(Option);
  std::size_t operator()(Positional);
  std::size_t operator()(Subcommand);
  std::size_t operator()(Unknown);

private:
  ArgMap map;
  Program const &spec;
  std::span<char const *> args;
  std::size_t current_positional_idx{};

  alternatives decide_type(std::size_t) const noexcept;
};

} // namespace parsing

} // namespace opzioni

#endif // OPZIONI_PARSING_H
