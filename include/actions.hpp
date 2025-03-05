#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <optional>
#include <string_view>

#include "arg.hpp"
#include "args_map.hpp"
#include "concepts.hpp"

namespace opz {

namespace actions {

template <concepts::Command Cmd, typename T>
void assign_to(ArgsMap<Cmd const> &map, std::string_view const name, T const && value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted) throw DuplicateAssignment(name);
}

template <concepts::Command Cmd>
void print_version(Cmd const &cmd) {
  fmt::print("{} {}\n", cmd.name, cmd.version);
  std::exit(0);
}

} // namespace actions

template <concepts::Command Cmd, typename T>
void apply_action(Cmd const &cmd, ArgsMap<Cmd const> &map, Arg<T> const &arg, std::optional<std::string_view> const value) noexcept {
  switch (arg.action) {
    case Action::ASSIGN:
      actions::assign_to(map, arg.name, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
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
