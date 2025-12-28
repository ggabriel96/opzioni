#include "opzioni/arg.hpp"

namespace opz {

std::string_view to_string(ArgKind const at) noexcept {
  switch (at) {
    case ArgKind::POS: return "positional";
    case ArgKind::OPT: return "option";
    case ArgKind::FLG: return "flag";
    default: return "unknown";
  }
}

} // namespace opz
