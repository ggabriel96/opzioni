#ifndef OPZIONI_EXCEPTIONS_H
#define OPZIONI_EXCEPTIONS_H

#include <stdexcept>

namespace opz {
class ArgumentNotFound : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

class UnknownArgument : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

class FlagHasValue : public std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

class MissingValue : public std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

class InvalidChoice : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

class ConversionError : public std::domain_error {
  using std::domain_error::domain_error;
};
} // namespace opz

#endif // OPZIONI_EXCEPTIONS_H
