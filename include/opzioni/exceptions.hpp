#ifndef OPZIONI_EXCEPTIONS_HPP
#define OPZIONI_EXCEPTIONS_HPP

#include <stdexcept>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni/cmd_fmt.hpp"

namespace opz {

// +-------------------+
// | programmer errors |
// +-------------------+

// Base class for exceptions thrown because of errors from the users of our library
class ProgrammerError : public std::logic_error {
public:

  using std::logic_error::logic_error;
};

class ArgumentNotFound : public ProgrammerError {
public:

  explicit ArgumentNotFound(std::string_view name)
    : ProgrammerError(fmt::format("Could not find argument `{}`", name)) {}
};

// +-------------+
// | user errors |
// +-------------+

// Base class for exceptions thrown because of errors from the users of the CLI program
class UserError : public std::runtime_error {
public:

  CmdFmt formatter;

  UserError(std::string const &msg, CmdFmt formatter) : std::runtime_error(msg), formatter(std::move(formatter)) {}
};

class MissingRequiredArgument : public UserError {
public:

  MissingRequiredArgument(std::string_view cmd_name, std::string_view name, CmdFmt const &formatter)
    : UserError(fmt::format("Missing required argument `{}` for command `{}`", name, cmd_name), formatter) {}
};

class UnexpectedPositional : public UserError {
public:

  UnexpectedPositional(
    std::string_view cmd_name, std::string_view name, std::size_t expected_amount, CmdFmt const &formatter
  )
    : UserError(
        fmt::format(
          "Unexpected positional argument for `{}`: `{}` (only {} are expected)", cmd_name, name, expected_amount
        ),
        formatter
      ) {}
};

class UnknownArguments : public UserError {
public:

  UnknownArguments(
    std::string_view cmd_name, std::vector<std::string_view> const &unknown_args, CmdFmt const &formatter
  )
    : UserError(
        fmt::format("Unknown arguments for `{}` command: `{}`", cmd_name, fmt::join(unknown_args, "`, `")), formatter
      ) {}
};

class UnknownSubcommand : public UserError {
public:

  UnknownSubcommand(std::string_view cmd_name, std::string_view subcmd_name, CmdFmt const &formatter)
    : UserError(fmt::format("Unknown subcommand `{}` for command `{}`", subcmd_name, cmd_name), formatter) {}
};

class ConflictingArguments : public UserError {
public:

  ConflictingArguments(
    std::string_view cmd_name, std::string_view arg_name, std::string_view other_arg_name, CmdFmt const &formatter
  )
    : UserError(
        fmt::format(
          "Argument `{}` conflicts with previously provided `{}` argument for command `{}`, please check help for more details",
          arg_name,
          other_arg_name,
          cmd_name
        ),
        formatter
      ) {}
};

// +------------------------------------------+
// | runtime errors to convert to user errors |
// +------------------------------------------+

class ConversionError : public std::runtime_error {
public:

  ConversionError(auto from, auto to) : std::runtime_error(fmt::format("Cannot convert `{}` to `{}`", from, to)) {}
};

class MissingValue : public std::runtime_error {
public:

  MissingValue(std::string_view name, std::size_t expected_amount, std::size_t received_amount)
    : std::runtime_error(
        fmt::format("Expected {} value(s) for argument `{}`, got {}", expected_amount, name, received_amount)
      ) {}
};

class UnexpectedValue : public std::runtime_error {
public:

  UnexpectedValue(std::string_view name, std::size_t expected_amount, std::size_t received_amount)
    : std::runtime_error(
        fmt::format("Expected {} value(s) for argument `{}`, got {}", expected_amount, name, received_amount)
      ) {}
};

} // namespace opz

#endif // OPZIONI_EXCEPTIONS_HPP
