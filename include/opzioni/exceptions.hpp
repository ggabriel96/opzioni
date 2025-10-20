#ifndef OPZIONI_EXCEPTIONS_HPP
#define OPZIONI_EXCEPTIONS_HPP

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni/cmd_fmt.hpp"

namespace opz {

// +-----------------+
// | consumer errors |
// +-----------------+

// Base class for exceptions thrown because of errors from the users of our library
class ConsumerError : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

class ArgumentNotFound : public ConsumerError {
public:
  explicit ArgumentNotFound(std::string_view name) : ConsumerError(fmt::format("Could not find argument `{}`", name)) {}
};

// +-------------+
// | user errors |
// +-------------+

// Base class for exceptions thrown because of errors from the users of the CLI program
class UserError : public std::runtime_error {
public:
  CmdFmt formatter;

  UserError(std::string const &msg, CmdFmt const &formatter) : std::runtime_error(msg), formatter(formatter) {}
};

class MissingRequiredArguments : public UserError {
public:
  MissingRequiredArguments(std::string_view cmd_name, std::ranges::range auto const &names, CmdFmt const &formatter)
    : UserError(
        fmt::format("Missing required arguments for `{}`: `{}`", cmd_name, fmt::join(names, "`, `")), formatter
      ) {}
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

class UnknownArgument : public UserError {
public:
  UnknownArgument(std::string_view cmd_name, std::string_view name, CmdFmt const &formatter)
    : UserError(fmt::format("Unknown argument for `{}`: `{}`", cmd_name, name), formatter) {}
};

class WrongType : public UserError {
public:
  WrongType(
    std::string_view cmd_name,
    std::string_view name,
    std::string_view expected_type,
    std::string_view received_type,
    CmdFmt const &formatter
  )
    : UserError(
        fmt::format(
          "Argument `{}` is a known {} of `{}`, but was provided as {}", name, expected_type, cmd_name, received_type
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
