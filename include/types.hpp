#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include "converters.hpp"
#include "memory.hpp"

#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace opzioni {

template <typename...> struct TypeList;

template <typename...> struct Prepend;
template <typename T, typename... Ts> struct Prepend<T, TypeList<Ts...>> { using type = TypeList<T, Ts...>; };

template <typename...> struct VariantOf;
template <typename... Ts> struct VariantOf<TypeList<Ts...>> { using type = std::variant<Ts...>; };

using BuiltinTypes = TypeList<bool, int, double, std::string, std::vector<int>>;
using OptionalBuiltinTypes = Prepend<std::monostate, BuiltinTypes>::type;
using BuiltinType = VariantOf<BuiltinTypes>::type;
using OptionalBuiltinType = VariantOf<OptionalBuiltinTypes>::type;

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

  template <typename T> T get_if_else(T &&desired, std::string name, T &&otherwise) const {
    // mainly intended for flags which are not only true or false
    if (args.contains(name))
      return desired;
    else
      return otherwise;
  }

  template <typename T> T value_or(std::string name, T &&otherwise) const {
    if (auto const arg = args.find(name); arg != args.end())
      return arg->second.as<T>();
    else
      return otherwise;
  }

  template <typename T> T as(std::string name) const {
    auto const arg = (*this)[name];
    return arg.as<T>();
  }

  auto size() const noexcept { return this->args.size(); }

  std::string cmd_name;
  memory::ValuePtr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

struct Arg;

namespace actions {
using signature = void (*)(ArgMap &, Arg const &, std::optional<std::string> const &);

template <typename T> void assign(ArgMap &, Arg const &, std::optional<std::string> const &);
} // namespace actions

struct Arg {
  std::string name{};
  std::string description{};
  bool is_required = false;
  OptionalBuiltinType default_value{};
  actions::signature action_fn = actions::assign<std::string>;

  Arg &help(std::string) noexcept;
  Arg &required() noexcept;
  Arg &action(actions::signature) noexcept;

  template <typename T> Arg &otherwise(T value) {
    default_value = std::move(value);
    is_required = false;
    if (action_fn == actions::assign<std::string>)
      action_fn = actions::assign<T>;
    return *this;
  }
};

namespace actions {
template <typename T> void assign(ArgMap &map, Arg const &arg, std::optional<std::string> const &parsed_value) {
  T value = parsed_value.has_value() ? convert<T>(*parsed_value) : std::get<T>(arg.default_value);
  map.args[arg.name] = ArgValue{value};
}

template <typename T> void append(ArgMap &map, Arg const &arg, std::optional<std::string> const &parsed_value) {
  T value = convert<T>(*parsed_value);
  auto list = map.args.find(arg.name);
  if (list != map.args.end()) {
    std::get<std::vector<T>>(list->second.value).push_back(value);
  } else {
    map.args[arg.name] = ArgValue{std::vector{value}};
  }
}
} // namespace actions

struct ParsedOption {
  std::string name;
  std::optional<std::string> value;
};

struct Program {
  std::string name{};
  std::string description{};
  std::string epilog{};

  std::map<std::string, memory::ValuePtr<Program>> cmds;
  std::vector<Arg> positional_args;
  std::map<std::string, Arg> flags;
  std::map<std::string, Arg> options;

  Program() = default;
  Program(std::string name) : name(name) {}
  Program(std::string name, std::string description, std::string epilog)
      : name(name), description(description), epilog(epilog) {}

  Program &help(std::string) noexcept;
  Program &with_epilog(std::string) noexcept;

  ArgMap operator()(int, char const *[]) const;

  Arg &pos(std::string);
  Arg &opt(std::string);
  Arg &flag(std::string);
  Program &cmd(std::string);

  bool is_flag(std::string const &) const noexcept;

  std::optional<std::string> is_positional(std::string const &) const noexcept;
  std::optional<std::string> is_long_flag(std::string const &) const noexcept;
  std::optional<std::string> is_short_flags(std::string const &) const noexcept;
  std::optional<ParsedOption> is_option(std::string const &) const noexcept;
  std::optional<decltype(cmds)::const_iterator> is_subcmd(std::string const &) const noexcept;
};

} // namespace opzioni

#endif // OPZIONI_TYPES_H
