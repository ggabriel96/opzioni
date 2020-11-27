#ifndef OPZIONI_EXCEPTIONS_H
#define OPZIONI_EXCEPTIONS_H

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

namespace opzioni {

// Base class for exceptions thrown because of errors from the users of our library
class ConsumerError : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

// Base class for exceptions thrown because of errors from the users of the CLI program
class UserError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// +-----------------+
// | consumer errors |
// +-----------------+

class ArgumentAlreadyExists : public ConsumerError {
public:
  using ConsumerError::ConsumerError;
};

class ArgumentNotFound : public ConsumerError {
public:
  ArgumentNotFound(std::string_view name) : ConsumerError(fmt::format("Could not find argument `{}`", name)) {}
};

// +-------------+
// | user errors |
// +-------------+

class ConversionError : public UserError {
public:
  using UserError::UserError;
};

class MissingValue : public UserError {
public:
  MissingValue(std::string_view name, std::size_t expected_amount, std::size_t received_amount)
      : UserError(
            fmt::format("Expected {} value(s) for argument `{}`, got {}", expected_amount, name, received_amount)) {}
};

class DuplicateAssignment : public UserError {
public:
  DuplicateAssignment(std::string_view name)
      : UserError(fmt::format("Attempt to assign argument `{}` failed because it was already set."
                              " Did you specify it more than once?",
                              name)) {}
};

class ParseError : public UserError {
public:
  using UserError::UserError;
};

class UnexpectedPositional : public UserError {
public:
  UnexpectedPositional(std::string_view name, std::size_t expected_amount)
      : UserError(fmt::format("Unexpected positional argument `{}`. This program expects {} positional arguments", name,
                              expected_amount)) {}
};

class UnknownArgument : public UserError {
public:
  UnknownArgument(std::string_view name) : UserError(fmt::format("Unknown argument `{}`", name)) {}
};

} // namespace opzioni

#endif // OPZIONI_EXCEPTIONS_H
