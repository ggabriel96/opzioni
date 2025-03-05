#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <optional>
#include <stdexcept>
#include <string_view>

#include "arg.hpp"
#include "args_map.hpp"
#include "concepts.hpp"

namespace opz {

namespace actions {

template <concepts::Command Cmd, typename T>
void assign_to(ArgsMap<Cmd const> &map, std::string_view const name, T const &&value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted) throw DuplicateAssignment(name);
}

template <concepts::Command Cmd, concepts::Integer I>
void count(ArgsMap<Cmd const> &map, std::string_view const name, I const &implicit_value) {
  auto [it, inserted] = map.args.try_emplace(name, implicit_value);
  if (!inserted) it->second = std::any_cast<I>(it->second) + implicit_value;
}

template <concepts::Command Cmd>
void print_help(Cmd const &cmd) {
  fmt::print("{} HELP PLACEHOLDER\n", cmd.name);
  std::exit(0);
}

template <concepts::Command Cmd>
void print_version(Cmd const &cmd) {
  fmt::print("{} {}\n", cmd.name, cmd.version);
  std::exit(0);
}

} // namespace actions

template <concepts::Command Cmd, typename T>
void apply_action(
  Cmd const &cmd, ArgsMap<Cmd const> &map, Arg<T> const &arg, std::optional<std::string_view> const value) {
  switch (arg.action) {
    case Action::ASSIGN:
      actions::assign_to(map, arg.name, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
      break;
    case Action::COUNT:
      if constexpr (concepts::Integer<T>) actions::count(map, arg.name, *arg.implicit_value);
      else {
        throw std::logic_error("TODO: apply_action called with non-integer type leaked to runtime");
      }
      break;
    case Action::PRINT_HELP:
      actions::print_help(cmd);
      break;
    case Action::PRINT_VERSION:
      actions::print_version(cmd);
      break;
    default:
      break;
  }
}

} // namespace opz

#endif // OPZIONI_ACTIONS_HPP
