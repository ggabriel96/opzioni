#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <any>
#include <map>
#include <optional>
#include <string_view>

#include "opzioni/arg.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/converters.hpp"

namespace opz::act {

namespace detail {

template <typename T>
void assign_to(std::map<std::string_view, std::any> &args_map, Arg<T, act::assign> const &arg, T const &&value) {
  auto const [_, inserted] = args_map.try_emplace(arg.name, value);
  if (!inserted) throw UnexpectedValue(arg.name, 1, 2);
}

} // namespace detail

template <typename T, typename Tag>
void process(
  std::map<std::string_view, std::any> &,
  Arg<T, Tag> const &,
  std::optional<std::string_view> const &,
  CmdInfoGetter &
);

template <concepts::Container C>
void process(
  std::map<std::string_view, std::any> &args_map,
  Arg<C, act::append> const &arg,
  std::optional<std::string_view> const &value,
  CmdInfoGetter &
) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  auto const converted_value = convert<typename C::value_type>(*value);
  if (auto it = args_map.find(arg.name); it != args_map.end()) {
    std::any_cast<C &>(it->second).emplace_back(converted_value);
  } else {
    detail::assign_to(args_map, arg, C{converted_value});
  }
}

template <typename T>
void process(
  std::map<std::string_view, std::any> &args_map,
  Arg<T, act::assign> const &arg,
  std::optional<std::string_view> const &value,
  CmdInfoGetter &
) {
  if (!value.has_value() && !arg.has_implicit()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map, arg, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
}

template <concepts::Integer I>
void process(
  std::map<std::string_view, std::any> &args_map,
  Arg<I, act::count> const &arg,
  std::optional<std::string_view> const &value,
  CmdInfoGetter &
) {
  if (value.has_value()) throw UnexpectedValue(arg.name, 0, 1);
  auto [it, inserted] = args_map.try_emplace(arg.name, *arg.implicit_value);
  if (!inserted) std::any_cast<I &>(it->second) += *arg.implicit_value;
}

template <concepts::Container C>
void process(
  std::map<std::string_view, std::any> &args_map,
  Arg<C, act::csv> const &arg,
  std::optional<std::string_view> const &value,
  CmdInfoGetter &
) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map, arg, convert<C>(*value));
}

} // namespace opz::act

#endif // OPZIONI_ACTIONS_HPP
