#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include "memory.hpp"
#include "types.hpp"

#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace opzioni {

namespace parsing {

ParsedOption parse_option(std::string const &) noexcept;

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

  ParseResult operator()();
  std::size_t operator()(DashDash);
  std::size_t operator()(Flag);
  std::size_t operator()(ManyFlags);
  std::size_t operator()(Option);
  std::size_t operator()(Positional);
  std::size_t operator()(Subcommand);
  std::size_t operator()(Unknown);

private:
  Program const &spec;
  std::span<char const *> args;
  ParseResult result;

  alternatives decide_type(std::size_t) const noexcept;
};

} // namespace parsing

} // namespace opzioni

#endif // OPZIONI_PARSING_H
