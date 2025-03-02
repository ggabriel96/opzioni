#include "arg.hpp"

namespace opz {

std::string_view ToString(ArgType at) noexcept {
  switch (at) {
    case ArgType::POS:
      return "positional";
    case ArgType::OPT:
      return "option";
    case ArgType::FLG:
      return "flag";
    default:
      return "unknown";
  }
}

} // namespace opz
