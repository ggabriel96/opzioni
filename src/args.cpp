#include "args.hpp"
#include "parsing.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

#include <fmt/format.h>

namespace opzioni {

Arg &Arg::help(std::string description) noexcept {
  this->description = description;
  return *this;
}

Arg &Arg::required() noexcept {
  this->is_required = true;
  return *this;
}

Arg &Arg::action(actions::signature action_fn) noexcept {
  this->action_fn = action_fn;
  return *this;
}

} // namespace opzioni
