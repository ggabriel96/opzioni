#include "args.hpp"

#include <fmt/format.h>

namespace opzioni {

template <> std::string Arg<ArgumentType::POSITIONAL>::format_usage() const noexcept {
  return fmt::format("<{}>", name);
}

template <> std::string Arg<ArgumentType::OPTION>::format_usage() const noexcept {
  return fmt::format("--{0} <{0}>", name);
}

template <> std::string Arg<ArgumentType::FLAG>::format_usage() const noexcept { return fmt::format("--{}", name); }

template <> std::string Arg<ArgumentType::POSITIONAL>::format_description() const noexcept {
  return fmt::format("{}", description);
}

template <> std::string Arg<ArgumentType::OPTION>::format_description() const noexcept {
  return fmt::format("{}", description);
}

template <> std::string Arg<ArgumentType::FLAG>::format_description() const noexcept {
  return fmt::format("{}", description);
  // auto const fmt_variant = [](auto const &var) { return fmt::format("{}", var); };
  // auto format =
  //     fmt::format("{}: {} (sets {} if present", format_usage(), description, std::visit(fmt_variant, *set_value));
  // if (default_value) {
  //   format += fmt::format(", default is {})", std::visit(fmt_variant, *default_value));
  // } else {
  //   format += fmt::format(")");
  // }
  // return format;
}

} // namespace opzioni
