#ifndef OPZIONI_H
#define OPZIONI_H

#include "converters.hpp"
#include "exceptions.hpp"
#include "memory.hpp"

#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opzioni {

constexpr char nl = '\n';

// +----------------------------------------+
// | related to type list and builtin types |
// +----------------------------------------+

template <typename...> struct TypeList;

template <typename...> struct VariantOf;
template <typename... Ts> struct VariantOf<TypeList<Ts...>> { using type = std::variant<Ts...>; };

using BuiltinTypes = TypeList<bool, int, double, std::string, std::vector<int>, std::vector<std::string>>;
using BuiltinType = VariantOf<BuiltinTypes>::type;

// +----------------------+
// | forward declarations |
// +----------------------+
enum struct ArgumentType { FLAG, OPTION, POSITIONAL };

struct ArgMap;
template <ArgumentType> struct Arg;
class Program;

using Flag = Arg<ArgumentType::FLAG>;
using Option = Arg<ArgumentType::OPTION>;
using Positional = Arg<ArgumentType::POSITIONAL>;

namespace actions {

template <ArgumentType type>
using signature = void (*)(Program const &, ArgMap &, Arg<type> const &, std::optional<std::string> const &);

template <typename T> void assign(Program const &, ArgMap &, Flag const &, std::optional<std::string> const &);
template <typename T> void assign(Program const &, ArgMap &, Option const &, std::optional<std::string> const &);
template <typename T> void assign(Program const &, ArgMap &, Positional const &, std::optional<std::string> const &);

template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Flag const &, std::optional<std::string> const &);
template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Option const &, std::optional<std::string> const &);
template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Positional const &, std::optional<std::string> const &);

void print_help(Program const &, ArgMap &, Flag const &, std::optional<std::string> const &);

} // namespace actions

// +-----------+
// | arguments |
// +-----------+

struct GatherAmount {
  std::size_t amount = 1;
};

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
  std::string format_long_usage() const noexcept;
};

template <ArgumentType type> void set_default(ArgMap &map, Arg<type> const &arg) noexcept {
  map.args[arg.name] = ArgValue{*arg.default_value};
}

template <ArgumentType type> bool operator<(Arg<type> const &lhs, Arg<type> const &rhs) noexcept {
  if (lhs.is_required == rhs.is_required) return lhs.name < rhs.name;
  return lhs.is_required && !rhs.is_required;
}

namespace parsing {

struct ParsedOption {
  std::string name;
  std::optional<std::string> value;
};

ParsedOption parse_option(std::string) noexcept;

struct Command {
  Program &program;
  std::size_t index;
};

struct DashDash {
  std::size_t index;
};

struct Flag {
  std::string name;
};

struct ManyFlags {
  std::string chars;
};

struct Option {
  ParsedOption arg;
  size_t index;
};

struct Positional {
  std::size_t index;
};

struct Unknown {
  std::size_t index;
};

using alternatives = std::variant<Command, DashDash, Flag, ManyFlags, Option, Positional, Unknown>;

class Parser {
public:
  Parser(Program const &spec, std::span<char const *> args) : spec(spec), args(args) {}

  ArgMap operator()();
  std::size_t operator()(parsing::Flag);
  std::size_t operator()(parsing::Option);
  std::size_t operator()(parsing::Command);
  std::size_t operator()(parsing::DashDash);
  std::size_t operator()(parsing::ManyFlags);
  std::size_t operator()(parsing::Positional);
  std::size_t operator()(parsing::Unknown);

private:
  ArgMap map;
  Program const &spec;
  std::span<char const *> args;
  std::size_t current_positional_idx{};

  parsing::alternatives decide_type(std::size_t) const noexcept;

  bool is_dash_dash(std::string const &) const noexcept;
  std::optional<std::string> looks_positional(std::string const &) const noexcept;

  bool would_be_positional(std::size_t) const noexcept;
};

} // namespace parsing

struct Program {
  std::string name{};
  std::string description{};
  std::string epilog{};

  std::vector<Flag> flags;
  std::vector<Option> options;
  std::vector<Positional> positionals;
  std::map<std::string, memory::ValuePtr<Program>> cmds;

  std::map<std::string, std::size_t> flags_idx;
  std::map<std::string, std::size_t> options_idx;

  Program() : Program({}, {}, {}) {}

  Program(std::string name) : Program(name, {}, {}) {}

  Program(std::string name, std::string description, std::string epilog)
      : name(name), description(description), epilog(epilog) {
    flag("help").help("Display this information").action(actions::print_help);
  }

  Program &help(std::string) noexcept;
  Program &with_epilog(std::string) noexcept;

  Flag &flag(std::string);
  Option &opt(std::string);
  Program &cmd(std::string);
  Positional &pos(std::string);

  ArgMap operator()(int, char const *[]);
  ArgMap operator()(std::span<char const *>);

  void set_defaults(ArgMap &) const noexcept;

  void print_usage(std::ostream & = std::cout) const noexcept;

  Program *is_command(std::string const &) const noexcept;
  bool is_flag(std::string const &) const noexcept;
  std::optional<std::string> is_long_flag(std::string const &) const noexcept;
  std::optional<std::string> is_short_flags(std::string const &) const noexcept;
  std::optional<parsing::ParsedOption> is_option(std::string const &) const noexcept;
};

// +------------+
// | formatting |
// +------------+

class HelpFormatter {
public:
  HelpFormatter(Program const &);

  std::size_t get_help_margin() const noexcept;

  std::string title() const noexcept;
  std::string usage() const noexcept;
  std::string help() const noexcept;
  std::string description() const noexcept;

private:
  std::string program_name;
  std::string program_description;
  std::string program_epilog;
  std::vector<Flag> flags;
  std::vector<Option> options;
  std::vector<Positional> positionals;

  std::string format_help(auto const &range, std::size_t const margin) const noexcept {
    using std::views::transform;
    auto const format = [margin](auto const &arg) {
      return fmt::format("    {:<{}}: {}", arg.format_long_usage(), margin, arg.format_description());
    };
    return fmt::format("{}", fmt::join(range | transform(format), "\n"));
  }
};

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

template <typename T>
void assign(Program const &, ArgMap &map, Flag const &arg, std::optional<std::string> const &parsed_value) {
  assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Option const &arg, std::optional<std::string> const &parsed_value) {
  if (parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Positional const &arg, std::optional<std::string> const &parsed_value) {
  assign_to(map, arg.name, convert<T>(*parsed_value));
}

// +--------+
// | append |
// +--------+

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Flag const &arg, std::optional<std::string> const &parsed_value) {
  Elem value = std::get<Elem>(*arg.set_value);
  append_to<Elem, Container>(map, arg.name, value);
}

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Option const &arg, std::optional<std::string> const &parsed_value) {
  if (parsed_value)
    append_to<Elem, Container>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem, Container>(map, arg.name, std::get<Elem>(*arg.set_value));
}

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Positional const &arg, std::optional<std::string> const &parsed_value) {
  Elem value = convert<Elem>(*parsed_value);
  append_to<Elem, Container>(map, arg.name, value);
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_H
