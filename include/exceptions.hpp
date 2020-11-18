#ifndef OPZIONI_EXCEPTIONS_H
#define OPZIONI_EXCEPTIONS_H

#include <stdexcept>

namespace opzioni {

// Base class for exceptions thrown because of errors from the users of our library
class ConsumerError : public std::logic_error {
  using std::logic_error::logic_error;
};

// Base class for exceptions thrown because of errors from the users of the CLI program
class UserError : public std::runtime_error {
  using std::runtime_error::runtime_error;
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

// +-----------------+
// | consumer errors |
// +-----------------+

class ArgumentAlreadyExists : public ConsumerError {
  using ConsumerError::ConsumerError;
};

class ArgumentNotFound : public ConsumerError {
  using ConsumerError::ConsumerError;
};

class UnknownArgument : public ConsumerError {
  using ConsumerError::ConsumerError;
};

} // namespace opzioni

#endif // OPZIONI_EXCEPTIONS_H
