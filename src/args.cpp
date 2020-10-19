#include "args.hpp"

#include <fmt/format.h>

namespace opzioni {

template<>
std::string Arg<ArgumentType::POSITIONAL>::format_usage() const noexcept { return fmt::format("<{}>", name); }

template<>
std::string Arg<ArgumentType::OPTION>::format_usage() const noexcept { return fmt::format("--{0} <{0}>", name); }

template<>
std::string Arg<ArgumentType::FLAG>::format_usage() const noexcept { return fmt::format("--{}", name); }

} // namespace opzioni
