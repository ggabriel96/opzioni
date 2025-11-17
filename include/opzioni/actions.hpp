#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <format>
#include <functional>
#include <iostream>
#include <string_view>
#include <variant>
#include <vector>

#include "opzioni/arg.hpp"
#include "opzioni/args_map.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/converters.hpp"
#include "opzioni/extra.hpp"
#include "opzioni/strings.hpp"
#include "opzioni/variant.hpp"

namespace opz::act {

using PosValueType = std::string_view;
using FlgValueType = std::size_t;
using OptValueType = std::reference_wrapper<std::vector<std::string_view> const>; // TODO: make it vector of optionals
                                                                                  // to support implicit value
using ArgValueTypes = TypeList<PosValueType, FlgValueType, OptValueType>;
static constexpr auto pos_idx = IndexOfType<0, PosValueType, ArgValueTypes>::value;
static constexpr auto flg_idx = IndexOfType<0, FlgValueType, ArgValueTypes>::value;
static constexpr auto opt_idx = IndexOfType<0, OptValueType, ArgValueTypes>::value;
using ArgValue = VariantOf<ArgValueTypes>::type;

template <int TupleIdx, concepts::Cmd Cmd, typename T, typename Tag>
void consume_arg(ArgsMap<Cmd const> &, Arg<T, Tag> const &arg, ArgValue const &, Cmd const &, ExtraInfo const &);

template <int TupleIdx, concepts::Cmd Cmd, typename T>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<T, act::assign> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  if (auto const vals = std::get_if<opt_idx>(&value); vals != nullptr && vals->get().size() > 1)
    throw UnexpectedValue(arg.name, 1, vals->get().size());
  std::visit(
    overloaded{
      [&args_map, &arg](PosValueType sv) { std::get<TupleIdx>(args_map.args) = convert<T>(sv); },
      [&args_map, &arg](FlgValueType) { std::get<TupleIdx>(args_map.args) = *arg.implicit_value; },
      [&args_map, &arg](OptValueType vec) {
        if (vec.get().size() > 1) throw UnexpectedValue(arg.name, 1, vec.get().size());
        std::get<TupleIdx>(args_map.args) = convert<T>(vec.get()[0]);
      },
    },
    value
  );
}

template <int TupleIdx, concepts::Cmd Cmd, concepts::Container C>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<C, act::append> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  auto &target = std::get<TupleIdx>(args_map.args);
  std::visit(
    overloaded{
      [&target](PosValueType sv) {
        if (target.has_value()) target->emplace_back(convert<typename C::value_type>(sv));
        else target.emplace(1, convert<typename C::value_type>(sv));
      },
      [&arg](FlgValueType flg_count) {
        throw std::logic_error(std::format("attempted to use a container type with flag `{}`", arg.name));
      },
      [&target](OptValueType vec) {
        C converted_values;
        for (auto const v : vec.get()) {
          converted_values.emplace_back(convert<typename C::value_type>(v));
        }
        if (target.has_value()) target->append_range(converted_values);
        else target.emplace(std::from_range_t{}, converted_values);
      },
    },
    value
  );
}

template <int TupleIdx, concepts::Cmd Cmd, concepts::Integer I>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<I, act::count> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  if (value.index() != flg_idx) {
    auto received_amount = 1;
    if (auto const vals = std::get_if<opt_idx>(&value); vals != nullptr) received_amount = vals->get().size();
    throw UnexpectedValue(arg.name, 0, received_amount);
  }
  auto const flg_amount = std::get<flg_idx>(value);
  std::get<TupleIdx>(args_map.args) = flg_amount;
}

template <int TupleIdx, concepts::Cmd Cmd, concepts::Container C>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<C, act::csv> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  auto &target = std::get<TupleIdx>(args_map.args);
  std::visit(
    overloaded{
      [&target](PosValueType sv) { target.emplace(std::from_range_t{}, convert<C>(sv)); },
      [&arg](FlgValueType) {
        throw std::logic_error(std::format("attempted to use the CSV action with flag `{}`", arg.name));
      },
      [&target, &arg](OptValueType vec) {
        if (vec.get().size() > 1) throw UnexpectedValue(arg.name, 1, vec.get().size());
        target.emplace(std::from_range_t{}, convert<C>(vec.get()[0]));
      },
    },
    value
  );
}

template <int TupleIdx, concepts::Cmd Cmd>
void consume_arg(
  ArgsMap<Cmd const> &,
  Arg<bool, act::print_help> const &,
  ArgValue const &,
  Cmd const &cmd,
  ExtraInfo const &extra_info
) {
  CmdFmt const formatter(cmd, extra_info);
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

template <int TupleIdx, concepts::Cmd Cmd>
void consume_arg(
  ArgsMap<Cmd const> &, Arg<bool, act::print_version> const &, ArgValue const &, Cmd const &cmd, ExtraInfo const &
) {
  fmt::print("{} {}\n", cmd.name, cmd.version);
  std::exit(0);
}

} // namespace opz::act

#endif // OPZIONI_ACTIONS_HPP
