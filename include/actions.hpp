#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "arg.hpp"
#include "args_map.hpp"
#include "concepts.hpp"
#include "formatting.hpp"

namespace opz {

namespace actions {

struct ExtraInfo {
  std::string_view parent_cmds_names{};
};

template <concepts::Command Cmd, typename T>
void assign_to(ArgsMap<Cmd const> &map, Arg<T> const &arg, T const &&value) {
  auto const [_, inserted] = map.args.try_emplace(arg.name, value);
  if (!inserted) throw UnexpectedValue(map.exec_path, arg.name, 1, 2);
}

template <concepts::Command Cmd, concepts::Container C>
void append_to(ArgsMap<Cmd const> &map, Arg<C> const &arg, typename C::value_type const &&value) {
  if (auto it = map.args.find(arg.name); it != map.args.end()) {
    std::any_cast<C &>(it->second).emplace_back(value);
  } else {
    assign_to(map, arg, C{value});
  }
}

template <concepts::Command Cmd, concepts::Integer I>
void count(ArgsMap<Cmd const> &map, std::string_view const name, I const &implicit_value) {
  auto [it, inserted] = map.args.try_emplace(name, implicit_value);
  if (!inserted) std::any_cast<I &>(it->second) += implicit_value;
}

template <concepts::Command Cmd, concepts::Container C>
void csv(ArgsMap<Cmd const> &map, Arg<C> const &arg, C const &&value) {
  assign_to(map, arg, std::forward<C const>(value));
}

template <concepts::Command Cmd>
void print_help(Cmd const &cmd, std::string_view const parent_cmds_names) {
  HelpFormatter formatter(cmd, parent_cmds_names);
  formatter.print_title();
  if (!cmd.introduction.empty()) {
    std::cout << nl;
    formatter.print_intro();
  }
  std::cout << nl;
  formatter.print_usage();
  std::cout << nl;
  formatter.print_help();
  std::cout << nl;
  formatter.print_details();
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
  Cmd const &cmd, ArgsMap<Cmd const> &map, Arg<T> const &arg, std::optional<std::string_view> const value,
  actions::ExtraInfo const extra_info) {
  switch (arg.action) {
    case Action::APPEND:
      if constexpr (concepts::Container<T>) {
        if (!value.has_value()) throw MissingValue(map.exec_path, arg.name, 1, 0);
        actions::append_to<Cmd, T>(map, arg, convert<typename T::value_type>(*value));
      } else throw std::logic_error("TODO: APPEND apply_action called with non-container type leaked to runtime");
      break;

    case Action::ASSIGN:
      if (!value.has_value() && !arg.has_implicit()) throw MissingValue(map.exec_path, arg.name, 1, 0);
      actions::assign_to(map, arg, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
      break;

    case Action::COUNT:
      if constexpr (concepts::Integer<T>) {
        if (value.has_value()) throw MissingValue(map.exec_path, arg.name, 1, 0);
        actions::count(map, arg.name, *arg.implicit_value);
      } else throw std::logic_error("TODO: COUNT apply_action called with non-integer type leaked to runtime");
      break;

    case Action::CSV:
      if constexpr (concepts::Container<T>) {
        if (!value.has_value()) throw MissingValue(map.exec_path, arg.name, 1, 0);
        actions::csv(map, arg, convert<T>(*value));
      } else throw std::logic_error("TODO: CSV apply_action called with non-container type leaked to runtime");
      break;

    case Action::PRINT_HELP:
      actions::print_help(cmd, extra_info.parent_cmds_names);
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
