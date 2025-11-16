#ifndef OPZIONI_ACTIONS_HPP
#define OPZIONI_ACTIONS_HPP

#include <any>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
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
using OptValueType = std::reference_wrapper<std::vector<std::string_view> const>;
using ArgValueTypes = TypeList<PosValueType, FlgValueType, OptValueType>;
static constexpr auto pos_idx = IndexOfType<0, PosValueType, ArgValueTypes>::value;
static constexpr auto flg_idx = IndexOfType<0, FlgValueType, ArgValueTypes>::value;
static constexpr auto opt_idx = IndexOfType<0, OptValueType, ArgValueTypes>::value;
using ArgValue = VariantOf<ArgValueTypes>::type;

namespace detail {

template <typename T>
void assign_to(std::map<std::string_view, std::any> &args_map, Arg<T, act::assign> const &arg, T const &&value) {
  auto const [_, inserted] = args_map.try_emplace(arg.name, value);
  if (!inserted) throw UnexpectedValue(arg.name, 1, 2);
}

} // namespace detail

// TODO: give it a better name or move to specific namespace
template <concepts::Cmd Cmd, typename T, typename Tag>
void process(
  ArgsMap<Cmd const> &, Arg<T, Tag> const &, std::optional<std::string_view> const &, Cmd const &, ExtraInfo const &
);

template <concepts::Cmd Cmd, concepts::Container C>
void process(
  ArgsMap<Cmd const> &args_map,
  Arg<C, act::append> const &arg,
  std::optional<std::string_view> const &value,
  Cmd const &,
  ExtraInfo const &
) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  auto const converted_value = convert<typename C::value_type>(*value);
  if (auto it = args_map.args.find(arg.name); it != args_map.args.end()) {
    std::any_cast<C &>(it->second).emplace_back(converted_value);
  } else {
    detail::assign_to(args_map.args, arg, C{converted_value});
  }
}

template <concepts::Cmd Cmd, typename T>
void process(
  ArgsMap<Cmd const> &args_map,
  Arg<T, act::assign> const &arg,
  std::optional<std::string_view> const &value,
  Cmd const &,
  ExtraInfo const &
) {
  if (!value.has_value() && !arg.has_implicit()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map.args, arg, arg.type != ArgType::FLG && value ? convert<T>(*value) : *arg.implicit_value);
}

template <concepts::Cmd Cmd, concepts::Integer I>
void process(
  ArgsMap<Cmd const> &args_map,
  Arg<I, act::count> const &arg,
  std::optional<std::string_view> const &value,
  Cmd const &,
  ExtraInfo const &
) {
  if (value.has_value()) throw UnexpectedValue(arg.name, 0, 1);
  auto [it, inserted] = args_map.args.try_emplace(arg.name, *arg.implicit_value);
  if (!inserted) std::any_cast<I &>(it->second) += *arg.implicit_value;
}

template <concepts::Cmd Cmd, concepts::Container C>
void process(
  ArgsMap<Cmd const> &args_map,
  Arg<C, act::csv> const &arg,
  std::optional<std::string_view> const &value,
  Cmd const &,
  ExtraInfo const &
) {
  if (!value.has_value()) throw MissingValue(arg.name, 1, 0);
  detail::assign_to(args_map.args, arg, convert<C>(*value));
}

template <concepts::Cmd Cmd>
void process(
  ArgsMap<Cmd const> &,
  Arg<bool, act::print_help> const &,
  std::optional<std::string_view> const &,
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

template <concepts::Cmd Cmd>
void process(
  ArgsMap<Cmd const> &args_map,
  Arg<bool, act::print_version> const &,
  std::optional<std::string_view> const &,
  Cmd const &cmd,
  ExtraInfo const &
) {
  fmt::print("{} {}\n", cmd.name, cmd.version);
  std::exit(0);
}

template <int TupleIdx, concepts::Cmd Cmd, typename T, typename Tag>
void consume_arg(ArgsMap<Cmd const> &, Arg<T, Tag> const &arg, ArgValue const &, Cmd const &, ExtraInfo const &) {
  std::cout << "base consume_arg " << arg.name << '\n';
}

template <int TupleIdx, concepts::Cmd Cmd, typename T>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<T, act::assign> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  // if (value.index() == flg_idx && !arg.has_implicit()) throw MissingValue(arg.name, 1, 0);
  if (auto const vals = std::get_if<opt_idx>(&value); vals != nullptr && vals->get().size() > 1)
    throw UnexpectedValue(arg.name, 1, vals->get().size());
  std::visit(
    overloaded{
      [&args_map, &arg](PosValueType sv) { std::get<TupleIdx>(args_map.args) = convert<T>(sv); },
      [&args_map, &arg](FlgValueType) { std::get<TupleIdx>(args_map.args) = *arg.implicit_value; },
      [&args_map, &arg](OptValueType vec) {
        // TODO: check that vec is not empty? check it has more than 1 element?
        std::get<TupleIdx>(args_map.args) = convert<T>(vec.get()[0]);
      },
    },
    value
  );
  std::cout << "assign consume_arg to " << arg.name << "=" << *std::get<TupleIdx>(args_map.args) << '\n';
}

template <int TupleIdx, concepts::Cmd Cmd, concepts::Container C>
void consume_arg(
  ArgsMap<Cmd const> &args_map, Arg<C, act::append> const &arg, ArgValue const &value, Cmd const &, ExtraInfo const &
) {
  // if (value.index() == flg_idx) throw MissingValue(arg.name, 1, 0); // TODO: does append work with positionals?
  // auto const converted_value = convert<typename C::value_type>(*value);
  // if (auto it = args_map.args.find(arg.name); it != args_map.args.end()) {
  //   std::any_cast<C &>(it->second).emplace_back(converted_value);
  // } else {
  //   detail::assign_to(args_map.args, arg, C{converted_value});
  // }
  auto &target = std::get<TupleIdx>(args_map.args);
  std::visit(
    overloaded{
      [&target](PosValueType sv) {
        if (target.has_value()) target->emplace_back(convert<typename C::value_type>(sv));
        else target.emplace(1, convert<typename C::value_type>(sv));
      },
      [&target, &arg](FlgValueType flg_count) {
        // if (target.has_value()) target->emplace_back(*arg.implicit_value);
        // else target.emplace(flg_count, *arg.implicit_value);
        // target.emplace(flg_count, *arg.implicit_value);
      },
      [&target, &arg](OptValueType vec) {
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
  // std::cout << "assign consume_arg to " << arg.name << "=" << *std::get<TupleIdx>(args_map.args) << '\n';
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

} // namespace opz::act

#endif // OPZIONI_ACTIONS_HPP
