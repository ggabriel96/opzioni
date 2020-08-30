#ifndef OPZIONI_ARGS_H
#define OPZIONI_ARGS_H

#include "converters.hpp"
#include "memory.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace opzioni {

enum struct ArgumentType { FLAG, OPTION, POSITIONAL };

// +----------------------+
// | forward declarations |
// +----------------------+

struct Arg;
struct ArgMap;

namespace actions {

using signature = void (*)(ArgMap &, Arg const &, std::optional<std::string> const &);

template <typename T> void assign(ArgMap &, Arg const &, std::optional<std::string> const &);

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

  std::string cmd_name;
  memory::ValuePtr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

struct Arg {
  std::string name{};
  std::string description{};
  bool is_required = false;
  std::optional<BuiltinType> default_value{};
  std::optional<BuiltinType> set_value{};
  actions::signature act = actions::assign<std::string>;

  Arg &help(std::string) noexcept;
  Arg &required() noexcept;
  Arg &action(actions::signature) noexcept;

  template <typename T> Arg &otherwise(T value) {
    default_value = std::move(value);
    is_required = false;
    // checking if act is the default because we cannot unconditionally
    // set it as the user might have set it before calling this function
    if (act == actions::assign<std::string>)
      act = actions::assign<T>;
    return *this;
  }

  template <typename T> Arg &set(T value) {
    set_value = std::move(value);
    if (act == actions::assign<bool>)
      act = actions::assign<T>;
    return *this;
  }
};

// +---------------------------+
// | implementation of actions |
// +---------------------------+

namespace actions {

template <typename T> void assign(ArgMap &map, Arg const &arg, std::optional<std::string> const &parsed_value) {
  T value = parsed_value.has_value() ? convert<T>(*parsed_value) : std::get<T>(*arg.set_value);
  map.args[arg.name] = ArgValue{value};
}

template <typename Elem, typename Container = std::vector<Elem>>
void append(ArgMap &map, Arg const &arg, std::optional<std::string> const &parsed_value) {
  Elem value = parsed_value.has_value() ? convert<Elem>(*parsed_value) : std::get<Elem>(*arg.set_value);
  if (auto list = map.args.find(arg.name); list != map.args.end()) {
    std::get<Container>(list->second.value).push_back(std::move(value));
  } else {
    map.args.emplace(std::make_pair(arg.name, Container{std::move(value)}));
  }
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_ARGS_H
