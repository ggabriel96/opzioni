#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <any>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>

#include "opzioni/arg.hpp"
#include "opzioni/args_map.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/converters.hpp"
#include "opzioni/extra.hpp"
#include "opzioni/strings.hpp"

namespace opz::act {

namespace detail {

template <typename T>
void assign_to(std::map<std::string_view, std::any> &args_map, Arg<T, act::assign> const &arg, T const &&value) {
  auto const [_, inserted] = args_map.try_emplace(arg.name, value);
  if (!inserted) throw UnexpectedValue(arg.name, 1, 2);
}

} // namespace detail

// TODO: give it a better name or move to specific namespace
template <concepts::Cmd Cmd, typename T, typename Tag>
void process(Cmd const &, ArgsMap<Cmd const> &, Arg<T, Tag> const &, std::optional<std::string_view> const &, ExtraInfo const &);

template <concepts::Cmd Cmd, concepts::Container C>
void process(Cmd const &, ArgsMap<Cmd const> &args_map, Arg<C, act::append> const &arg, std::optional<std::string_view> const &value, ExtraInfo const &) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  auto const converted_value = convert<typename C::value_type>(*value);
  if (auto it = args_map.args.find(arg.name); it != args_map.args.end()) {
    std::any_cast<C &>(it->second).emplace_back(converted_value);
  } else {
    detail::assign_to(args_map.args, arg, C{converted_value});
  }
}

template <concepts::Cmd Cmd, typename T>
void process(Cmd const &, ArgsMap<Cmd const> &args_map, Arg<T, act::assign> const &arg, std::optional<std::string_view> const &value, ExtraInfo const &) {
  if (!value.has_value() && !arg.has_implicit()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map.args, arg, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
}

template <concepts::Cmd Cmd, concepts::Integer I>
void process(Cmd const &, ArgsMap<Cmd const> &args_map, Arg<I, act::count> const &arg, std::optional<std::string_view> const &value, ExtraInfo const &) {
  if (value.has_value()) throw UnexpectedValue(arg.name, 0, 1);
  auto [it, inserted] = args_map.args.try_emplace(arg.name, *arg.implicit_value);
  if (!inserted) std::any_cast<I &>(it->second) += *arg.implicit_value;
}

template <concepts::Cmd Cmd, concepts::Container C>
void process(Cmd const &, ArgsMap<Cmd const> &args_map, Arg<C, act::csv> const &arg, std::optional<std::string_view> const &value, ExtraInfo const &) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map.args, arg, convert<C>(*value));
}

template<concepts::Cmd Cmd>
void process(Cmd const &cmd, ArgsMap<Cmd const> &, Arg<bool, act::print_help> const &, std::optional<std::string_view> const &, ExtraInfo const &info) {
  auto const &formatter = CmdInfo(cmd, info.parent_cmds_names);
  formatter.print_title();
  if (!formatter.introduction.empty()) {
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

template<concepts::Cmd Cmd>
void process(Cmd const &cmd, ArgsMap<Cmd const> &args_map, Arg<bool, act::print_version> const &, std::optional<std::string_view> const &, ExtraInfo const &) {
  fmt::print("{} {}\n", cmd.name, cmd.version);
  std::exit(0);
}

} // namespace opz::act

#endif // OPZIONI_ACTIONS_HPP
