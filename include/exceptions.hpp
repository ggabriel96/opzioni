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
} // namespace opz

#endif // OPZIONI_EXCEPTIONS_H
