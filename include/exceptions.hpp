#ifndef OPZIONI_EXCEPTIONS_H
#define OPZIONI_EXCEPTIONS_H

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

namespace opzioni {

// Base class for exceptions thrown because of errors from the users of our library
class ConsumerError : public std::logic_error {
  using std::logic_error::logic_error;
};

// Base class for exceptions thrown because of errors from the users of the CLI program
class UserError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// +-----------------+
// | consumer errors |
// +-----------------+

class ArgumentAlreadyExists : public ConsumerError {
  using ConsumerError::ConsumerError;
};

class ArgumentNotFound : public ConsumerError {
public:
  ArgumentNotFound(std::string_view name) : ConsumerError(fmt::format("Could not find argument `{}`", name)) {}
};

class UnknownArgument : public ConsumerError {
  using ConsumerError::ConsumerError;
};

// +-------------+
// | user errors |
// +-------------+

class ConversionError : public UserError {
  using UserError::UserError;
};

class MissingValue : public UserError {
  using UserError::UserError;
};

struct DuplicateAssignment : public UserError {
  using UserError::UserError;
};

struct ParseError : public UserError {
  using UserError::UserError;
};

} // namespace opzioni

#endif // OPZIONI_EXCEPTIONS_H
