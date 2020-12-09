#ifndef OPZIONI_H
#define OPZIONI_H

#include "converters.hpp"
#include "exceptions.hpp"
#include "string.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opzioni {

// +----------------------------------------+
// | related to type list and builtin types |
// +----------------------------------------+

template <typename...>
struct TypeList;

template <typename...>
struct VariantOf;

template <typename... Ts>
struct VariantOf<TypeList<Ts...>> {
  using type = std::variant<Ts...>;
};

using BuiltinTypes = TypeList<bool, int, double, std::string, std::vector<int>, std::vector<std::string>>;
using BuiltinType = VariantOf<BuiltinTypes>::type;

std::string builtin2str(BuiltinType const &) noexcept;

// +----------------------+
// | forward declarations |
// +----------------------+
enum struct ArgumentType { FLAG, OPTION, POSITIONAL };

struct ArgMap;

template <ArgumentType>
struct Arg;

class Program;

using Flg = Arg<ArgumentType::FLAG>;
using Opt = Arg<ArgumentType::OPTION>;
using Pos = Arg<ArgumentType::POSITIONAL>;

// +----------------+
// | error handlers |
// +----------------+
using error_handler = int (*)(Program const &, UserError const &) noexcept;

int print_error(Program const &, UserError const &) noexcept;
int print_error_and_usage(Program const &, UserError const &) noexcept;

namespace actions {

template <ArgumentType type>
using signature = void (*)(Program const &, ArgMap &, Arg<type> const &, std::optional<std::string_view> const);

template <typename T>
void assign(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);

template <typename T>
void assign(Program const &, ArgMap &, Opt const &, std::optional<std::string_view> const);

template <typename T>
void assign(Program const &, ArgMap &, Pos const &, std::optional<std::string_view> const);

template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);
template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Opt const &, std::optional<std::string_view> const);
template <typename Elem, typename Container = std::vector<Elem>>
void append(Program const &, ArgMap &, Pos const &, std::optional<std::string_view> const);

void print_help(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);

} // namespace actions

// +-----------+
// | arguments |
// +-----------+

struct GatherAmount {
  std::size_t amount = 1;
};

struct ArgValue {
  BuiltinType value{};

  template <typename T>
  T as() const {
    return std::get<T>(value);
  }

  template <typename T>
  operator T() const {
    return as<T>();
  }
};

struct ArgMap {
  ArgValue operator[](std::string_view name) const {
    if (!args.contains(name))
      throw ArgumentNotFound(name);
    return args.at(name);
  }

  template <typename T>
  T as(std::string_view name) const {
    auto const arg = (*this)[name];
    return arg.as<T>();
  }

  bool has(std::string_view name) const noexcept { return args.contains(name); }

  ArgMap *cmd(std::string_view name) {
    if (cmd_name == name)
      return cmd_args.get();
    return nullptr;
  }

  ArgMap const *cmd(std::string_view name) const {
    if (cmd_name == name)
      return cmd_args.get();
    return nullptr;
  }

  auto size() const noexcept { return this->args.size(); }

  void reset() noexcept {
    args.clear();
    exec_path = "";
    cmd_name = "";
    cmd_args = nullptr;
  }

  std::string exec_path{};
  std::string cmd_name{};
  std::shared_ptr<ArgMap> cmd_args;
  std::map<std::string_view, ArgValue> args;
};

template <ArgumentType type>
struct Arg {
  std::string_view name{};
  std::conditional_t<type != ArgumentType::POSITIONAL, std::string_view, std::monostate> abbrev{};
  std::string_view description{};
  bool is_required = false;
  std::optional<BuiltinType> default_value{};
  std::conditional_t<type != ArgumentType::POSITIONAL, std::optional<BuiltinType>, std::monostate> set_value{};
  actions::signature<type> action_fn = actions::assign<std::string>;
  std::conditional_t<type != ArgumentType::FLAG, GatherAmount, std::monostate> gather_n{};

  Arg<type> &aka(char const (&abbrev)[2]) noexcept requires(type != ArgumentType::POSITIONAL) {
    this->abbrev = abbrev;
    return *this;
  }

  Arg<type> &help(std::string_view description) noexcept {
    this->description = description;
    return *this;
  }

  Arg<type> &required() noexcept {
    this->is_required = true;
    return *this;
  }

  Arg<type> &action(actions::signature<type> action_fn) noexcept {
    this->action_fn = action_fn;
    return *this;
  }

  template <typename T = std::string>
  Arg<type> &gather() noexcept requires(type != ArgumentType::FLAG) {
    return gather<T>(0);
  }

  template <typename T = std::string>
  Arg<type> &gather(std::size_t gather_n) noexcept requires(type != ArgumentType::FLAG) {
    this->gather_n = {gather_n};
    action_fn = actions::append<T>;
    return *this;
  }

  template <typename T>
  Arg<type> &otherwise(T value) {
    default_value = std::move(value);
    is_required = false;
    // checking if action_fn is the default because we cannot unconditionally
    // set it as the user might have set it before calling this function
    if (action_fn == static_cast<actions::signature<type>>(actions::assign<std::string>))
      action_fn = actions::assign<T>;
    return *this;
  }

  template <typename T>
  Arg<type> &set(T value) requires(type != ArgumentType::POSITIONAL) {
    set_value = std::move(value);
    if (action_fn == static_cast<actions::signature<type>>(actions::assign<bool>) ||
        action_fn == static_cast<actions::signature<type>>(actions::assign<std::string>))
      action_fn = actions::assign<T>;
    return *this;
  }

  bool has_abbrev() const noexcept requires(type != ArgumentType::POSITIONAL) { return !abbrev.empty(); }

  std::string format_base_usage() const noexcept;
  std::string format_usage() const noexcept;
  std::string format_help_usage() const noexcept;
  std::string format_help_description() const noexcept;
};

template <ArgumentType type>
void set_default(ArgMap &map, Arg<type> const &arg) noexcept {
  map.args[arg.name] = ArgValue{*arg.default_value};
}

template <ArgumentType type>
bool operator<(Arg<type> const &lhs, Arg<type> const &rhs) noexcept {
  if (lhs.is_required == rhs.is_required)
    return lhs.name < rhs.name;
  return lhs.is_required && !rhs.is_required;
}

struct Cmd {
  std::string_view name{};
  Program *program;

  std::string format_help_usage() const noexcept;
  std::string format_help_description() const noexcept;

  auto operator<=>(Cmd const &other) const noexcept { return name <=> other.name; }
};

// +-------------+
// | Arg helpers |
// +-------------+

Cmd cmd(std::string_view, Program *) noexcept;
Pos pos(std::string_view) noexcept;
Opt opt(std::string_view) noexcept;
Opt opt(std::string_view, char const (&)[2]) noexcept;
Flg flg(std::string_view) noexcept;
Flg flg(std::string_view, char const (&)[2]) noexcept;

struct ParsedOption {
  std::string_view name;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

ParsedOption parse_option(std::string_view const) noexcept;

class Program {
public:
  std::string_view title{};
  std::string_view introduction{};
  std::string_view description{};
  std::string_view path{};

  std::size_t msg_width = 100;
  opzioni::error_handler error_handler = print_error;
  bool has_auto_help{false};

  std::vector<Flg> flags;
  std::vector<Opt> options;
  std::vector<Pos> positionals;
  std::vector<Cmd> cmds;

  std::map<std::string_view, std::size_t> cmds_idx;
  std::map<std::string_view, std::size_t> flags_idx;
  std::map<std::string_view, std::size_t> options_idx;

  Program &intro(std::string_view) noexcept;
  Program &details(std::string_view) noexcept;

  Program &max_width(std::size_t) noexcept;
  Program &on_error(opzioni::error_handler) noexcept;
  Program &auto_help() noexcept;
  Program &auto_help(actions::signature<ArgumentType::FLAG>) noexcept;

  Program &add(Flg);
  Program &add(Opt);
  Program &add(Pos);
  Program &add(Cmd);

  ArgMap operator()(int, char const *[]);
  ArgMap operator()(std::span<char const *>);

  template <typename T>
  Program &operator+(T arg) {
    return add(arg);
  }

private:
  ArgMap parse(std::span<char const *>) const;
  void check_contains_required(ArgMap const &) const;
  void set_defaults(ArgMap &) const noexcept;

  bool contains_pos_or_cmd(std::string_view const) const noexcept;
  bool contains_opt_or_flag(std::string_view const) const noexcept;

  bool is_dash_dash(std::string_view const) const noexcept;
  Cmd const *is_command(std::string_view const) const noexcept;
  bool looks_positional(std::string_view const) const noexcept;
  std::string_view is_short_flags(std::string_view const) const noexcept;
  std::string_view is_long_flag(std::string_view const) const noexcept;
  std::optional<ParsedOption> is_option(std::string_view const) const noexcept;
  bool is_flag(std::string_view const) const noexcept;

  std::size_t assign_command(ArgMap &, std::span<char const *>, Cmd const &) const;
  std::size_t assign_positional(ArgMap &, std::span<char const *>, std::size_t const) const;
  std::size_t assign_many_flags(ArgMap &, std::string_view) const;
  std::size_t assign_flag(ArgMap &, std::string_view) const;
  std::size_t assign_option(ArgMap &, std::span<char const *>, ParsedOption const) const;
};

// +-----------+
// | utilities |
// +-----------+

void print_full_help(Program const &, std::ostream & = std::cout) noexcept;

// +------------+
// | formatting |
// +------------+

class HelpFormatter {
public:
  HelpFormatter(Program const &, std::ostream &);

  void print_title() const noexcept;
  void print_intro() const noexcept;
  void print_long_usage() const noexcept;
  void print_help() const noexcept;
  void print_description() const noexcept;

  std::size_t help_padding_size() const noexcept;

private:
  std::ostream &out;
  std::size_t const max_width;
  std::string const program_title;
  std::string const program_introduction;
  std::string const program_description;
  std::string const program_path;
  std::vector<Flg> flags;
  std::vector<Opt> options;
  std::vector<Pos> positionals;
  std::vector<Cmd> cmds;

  void print_arg_help(auto const &arg, std::string_view const padding) const noexcept {
    using std::views::drop;
    auto const description = arg.format_help_description();
    // -8 because we'll later print a left margin of 8 spaces (4 of indentation, 4 of alignment)
    auto const lines = limit_within(description, max_width - padding.size() - 8);
    // -4 again because we'll shift it 4 spaces into the padding, invading it,
    // so we can have the possible following lines indented
    out << fmt::format("    {:<{}} {}\n", arg.format_help_usage(), padding.size() - 4, fmt::join(lines.front(), " "));
    for (auto const &line : lines | drop(1)) {
      out << fmt::format("    {} {}\n", padding, fmt::join(line, " "));
    };
  }
};

// +---------------------------+
// | implementation of actions |
// +---------------------------+

namespace actions {

template <typename T>
void assign_to(ArgMap &map, std::string_view const name, T value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted)
    throw DuplicateAssignment(name);
}

template <typename Elem, typename Container>
void append_to(ArgMap &map, std::string_view const name, Elem value) {
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
void assign(Program const &, ArgMap &map, Flg const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Opt const &arg, std::optional<std::string_view> const parsed_value) {
  if (parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(*arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Pos const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<T>(*parsed_value));
}

// +--------+
// | append |
// +--------+

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Flg const &arg, std::optional<std::string_view> const parsed_value) {
  Elem value = std::get<Elem>(*arg.set_value);
  append_to<Elem, Container>(map, arg.name, value);
}

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Opt const &arg, std::optional<std::string_view> const parsed_value) {
  if (parsed_value)
    append_to<Elem, Container>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem, Container>(map, arg.name, std::get<Elem>(*arg.set_value));
}

template <typename Elem, typename Container>
void append(Program const &, ArgMap &map, Pos const &arg, std::optional<std::string_view> const parsed_value) {
  Elem value = convert<Elem>(*parsed_value);
  append_to<Elem, Container>(map, arg.name, value);
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_H
