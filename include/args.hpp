#ifndef OPZIONI_ARGS_H
#define OPZIONI_ARGS_H

#include "converters.hpp"
#include "memory.hpp"

#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace opzioni {

enum struct ArgumentType { FLAG, OPTION, POSITIONAL };

// +----------------------+
// | forward declarations |
// +----------------------+

struct ArgMap;
template <ArgumentType> struct Arg;

using Flag = Arg<ArgumentType::FLAG>;
using Option = Arg<ArgumentType::OPTION>;
using Positional = Arg<ArgumentType::POSITIONAL>;

namespace actions {

template <ArgumentType type>
using signature = void (*)(ArgMap &, Arg<type> const &, std::optional<std::string> const &);

template <typename T> void assign(ArgMap &, Flag const &, std::optional<std::string> const &);
template <typename T> void assign(ArgMap &, Option const &, std::optional<std::string> const &);
template <typename T> void assign(ArgMap &, Positional const &, std::optional<std::string> const &);

template <typename Elem, typename Container = std::vector<Elem>>
void append(ArgMap &, Flag const &, std::optional<std::string> const &);
template <typename Elem, typename Container = std::vector<Elem>>
void append(ArgMap &, Option const &, std::optional<std::string> const &);
template <typename Elem, typename Container = std::vector<Elem>>
void append(ArgMap &, Positional const &, std::optional<std::string> const &);

} // namespace actions

// +----------------------------------------+
// | related to type list and builtin types |
// +----------------------------------------+

template <typename...> struct TypeList;

template <typename...> struct VariantOf;
template <typename... Ts> struct VariantOf<TypeList<Ts...>> { using type = std::variant<Ts...>; };

using BuiltinTypes = TypeList<bool, int, double, std::string, std::vector<int>, std::vector<std::string>>;
using BuiltinType = VariantOf<BuiltinTypes>::type;

// +-----------+
// | arguments |
// +-----------+

struct ArgValue {
  BuiltinType value{};

  template <typename T> T as() const { return std::get<T>(value); }
};

struct ArgMap {
  ArgValue operator[](std::string name) const {
    if (!args.contains(name))
      throw UnknownArgument(fmt::format("Could not find argument `{}`", name));
    return args.at(name);
  }

  template <typename T> T as(std::string name) const {
    auto const arg = (*this)[name];
    return arg.as<T>();
  }

  bool has(std::string name) const noexcept { return args.contains(name); }

  auto size() const noexcept { return this->args.size(); }

  void reset() noexcept {
    args.clear();
    subcmd = nullptr;
    cmd_name = std::string();
  }

  std::string cmd_name;
  memory::ValuePtr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

struct GatherAmount {
  std::size_t amount = 1;
};

template <ArgumentType type> struct Arg {
  std::string name{};
  std::string description{};
  bool is_required = false;
  std::optional<BuiltinType> default_value{};
  std::conditional_t<type != ArgumentType::POSITIONAL, std::optional<BuiltinType>, std::monostate> set_value{};
  actions::signature<type> act = actions::assign<std::string>;
  std::conditional_t<type != ArgumentType::FLAG, GatherAmount, std::monostate> gather_n{};

  Arg<type> &help(std::string description) noexcept {
    this->description = description;
    return *this;
  }

  Arg<type> &required() noexcept {
    this->is_required = true;
    return *this;
  }

  Arg<type> &action(actions::signature<type> act) noexcept {
    this->act = act;
    return *this;
  }

  template <typename T = std::string> Arg<type> &gather() noexcept requires(type != ArgumentType::FLAG) {
    return gather<T>(0);
  }

  template <typename T = std::string>
  Arg<type> &gather(std::size_t gather_n) noexcept requires(type != ArgumentType::FLAG) {
    this->gather_n = {gather_n};
    act = actions::append<T>;
    return *this;
  }

  template <typename T> Arg<type> &otherwise(T value) {
    default_value = std::move(value);
    is_required = false;
    // checking if act is the default because we cannot unconditionally
    // set it as the user might have set it before calling this function
    if (act == static_cast<actions::signature<type>>(actions::assign<std::string>))
      act = actions::assign<T>;
    return *this;
  }

  template <typename T> Arg<type> &set(T value) requires(type != ArgumentType::POSITIONAL) {
    set_value = std::move(value);
    if (act == static_cast<actions::signature<type>>(actions::assign<bool>) ||
        act == static_cast<actions::signature<type>>(actions::assign<std::string>))
      act = actions::assign<T>;
    return *this;
  }

  std::string format_description() const noexcept;
  std::string format_usage() const noexcept;
};

template <ArgumentType type> void set_default(ArgMap &map, Arg<type> const &arg) noexcept {
  map.args[arg.name] = ArgValue{*arg.default_value};
}

// +---------------------------+
// | implementation of actions |
// +---------------------------+

namespace actions {

template <typename T> void assign_to(ArgMap &map, std::string const &name, T value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted)
    throw DuplicateAssignment(fmt::format(
        "Attempted to assign argument `{}` but it was already set. Did you specify it more than once?", name));
}

template <typename Elem, typename Container> void append_to(ArgMap &map, std::string const &name, Elem value) {
  if (auto list = map.args.find(name); list != map.args.end()) {
    std::get<Container>(list->second.value).emplace_back(std::move(value));
  } else {
    assign_to(map, name, Container{std::move(value)});
  }
}

// +--------+
// | assign |
// +--------+

template <typename T> void assign(ArgMap &map, Flag const &arg, std::optional<std::string> const &parsed_value) {
  assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T> void assign(ArgMap &map, Option const &arg, std::optional<std::string> const &parsed_value) {
  if (parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T> void assign(ArgMap &map, Positional const &arg, std::optional<std::string> const &parsed_value) {
  assign_to(map, arg.name, convert<T>(*parsed_value));
}

// +--------+
// | append |
// +--------+

template <typename Elem, typename Container>
void append(ArgMap &map, Flag const &arg, std::optional<std::string> const &parsed_value) {
  Elem value = std::get<Elem>(*arg.set_value);
  append_to<Elem, Container>(map, arg.name, value);
}

template <typename Elem, typename Container>
void append(ArgMap &map, Option const &arg, std::optional<std::string> const &parsed_value) {
  if (parsed_value)
    append_to<Elem, Container>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem, Container>(map, arg.name, std::get<Elem>(*arg.set_value));
}

template <typename Elem, typename Container>
void append(ArgMap &map, Positional const &arg, std::optional<std::string> const &parsed_value) {
  Elem value = convert<Elem>(*parsed_value);
  append_to<Elem, Container>(map, arg.name, value);
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_ARGS_H
