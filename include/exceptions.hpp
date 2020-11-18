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

class ConversionError : public UserError {
  using UserError::UserError;
};

class MissingValue : public UserError {
  using UserError::UserError;
};

struct DuplicateAssignment : public UserError {
  using UserError::UserError;
};

// has mixed usage, will fix later...
class UnknownArgument : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

class ArgumentAlreadyExists : public ConsumerError {
  using ConsumerError::ConsumerError;
};

} // namespace opzioni

#endif // OPZIONI_EXCEPTIONS_H
