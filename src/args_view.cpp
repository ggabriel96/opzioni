#include "args_view.hpp"

#include <print>

namespace opz {

void ArgsView::print_debug() const noexcept {
  std::print("positionals ({}):", positionals.size());
  for (auto &&p : positionals) {
    std::print(" {}", p);
  }
  std::print("\n");

  std::print("options & flags:\n");
  for (auto &&o : options) {
    std::print("- {} = {}\n", o.first, o.second.value_or("<unset>"));
  }
}

} // namespace opz
