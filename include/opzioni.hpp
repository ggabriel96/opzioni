#ifndef OPZIONI_H
#define OPZIONI_H

#include "converters.hpp"
#include "exceptions.hpp"
#include "string.hpp"

#include <algorithm>
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
struct Concat;

template <typename... Lhs, typename... Rhs>
struct Concat<TypeList<Lhs...>, TypeList<Rhs...>> {
  using type = TypeList<Lhs..., Rhs...>;
};

template <typename...>
struct VariantOf;

template <typename... Ts>
struct VariantOf<TypeList<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename...>
struct VectorOf;

template <typename... Ts>
struct VectorOf<TypeList<Ts...>> {
  using type = TypeList<std::vector<Ts>...>;
};

// types that may be used as default_value and set_value
using ScalarTypes = TypeList<std::monostate, bool, int, double, std::string_view>;
using BuiltinType = VariantOf<ScalarTypes>::type;

// types that may be the result of parsing the CLI
using ExternalTypes = Concat<ScalarTypes, VectorOf<ScalarTypes>::type>::type;
using ExternalType = VariantOf<ExternalTypes>::type;

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

template <typename Elem>
void append(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);
template <typename Elem>
void append(Program const &, ArgMap &, Opt const &, std::optional<std::string_view> const);
template <typename Elem>
void append(Program const &, ArgMap &, Pos const &, std::optional<std::string_view> const);

template <typename T>
void csv(Program const &, ArgMap &, Opt const &, std::optional<std::string_view> const);
template <typename T>
void csv(Program const &, ArgMap &, Pos const &, std::optional<std::string_view> const);

void print_help(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);

void print_version(Program const &, ArgMap &, Flg const &, std::optional<std::string_view> const);

} // namespace actions

// +-----------+
// | arguments |
// +-----------+

struct GatherAmount {
  std::size_t amount = 1;
};

struct ArgValue {
  ExternalType value{};

  template <typename T>
  T as() const {
    return std::get<T>(value);
  }

  template <typename T>
  operator T() const {
    return as<T>();
  }
};

class ArgValueSetter {
public:
  ArgValueSetter(ArgValue &arg) : arg(arg) {}

  template <typename T>
  auto operator()(T value) {
    this->arg.value = value;
  }

private:
  ArgValue &arg;
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

template <typename T>
void set_empty_vector(ArgValue &arg) noexcept {
  arg.value = std::vector<T>{};
}

using default_value_setter = void (*)(ArgValue &);

template <ArgumentType type>
class Arg {
public:
  std::string_view name{};
  std::conditional_t<type != ArgumentType::POSITIONAL, std::string_view, std::monostate> abbrev{};
  std::string_view description{};
  bool is_required = false;
  BuiltinType default_value{};
  std::conditional_t<type != ArgumentType::POSITIONAL, BuiltinType, std::monostate> set_value{};
  actions::signature<type> action_fn = actions::assign<std::string_view>;
  std::conditional_t<type != ArgumentType::FLAG, GatherAmount, std::monostate> gather_info{};
  default_value_setter default_setter = nullptr;

  consteval Arg(std::string_view name) requires(type == ArgumentType::FLAG) : Arg(name, {}) {}

  consteval Arg(std::string_view name, std::string_view abbrev) requires(type == ArgumentType::FLAG)
      : name(name), abbrev(abbrev), set_value(true), action_fn(actions::assign<bool>) {
    if (!abbrev.empty() && abbrev.length() != 1) {
      throw "abbreviations must be a single letter"; // placeholder
    }
  }

  consteval Arg(std::string_view name) requires(type == ArgumentType::OPTION) : Arg(name, {}) {}

  consteval Arg(std::string_view name, std::string_view abbrev) requires(type == ArgumentType::OPTION)
      : name(name), abbrev(abbrev) {
    if (!abbrev.empty() && abbrev.length() != 1) {
      throw "abbreviations must be a single letter"; // placeholder
    }
  }

  consteval Arg(std::string_view name) requires(type == ArgumentType::POSITIONAL) : name(name), is_required(true) {}

  consteval Arg<type> action(actions::signature<type> action_fn) const noexcept {
    auto arg = *this;
    arg.action_fn = action_fn;
    return arg;
  }

  template <typename T>
  consteval Arg<type> append() const noexcept {
    auto arg = *this;
    arg.action_fn = actions::append<T>;
    arg.default_setter = set_empty_vector<T>;
    return arg;
  }

  template <typename T>
  consteval Arg<type> csv_of() const noexcept {
    auto arg = *this;
    arg.action_fn = actions::csv<T>;
    arg.default_setter = set_empty_vector<T>;
    return arg;
  }

  template <typename T = std::string_view>
  consteval Arg<type> gather(std::size_t amount) const noexcept requires(type != ArgumentType::FLAG) {
    auto arg = *this;
    arg.gather_info.amount = amount;
    arg.action_fn = actions::append<T>;
    if (amount != 1)
      arg.default_setter = set_empty_vector<T>;
    return arg;
  }

  template <typename T = std::string_view>
  consteval Arg<type> gather() const noexcept requires(type != ArgumentType::FLAG) {
    return gather<T>(0);
  }

  consteval Arg<type> help(std::string_view description) const noexcept {
    auto arg = *this;
    arg.description = description;
    return arg;
  }

  template <typename T>
  consteval Arg<type> of() const noexcept {
    auto arg = *this;
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  template <typename T>
  consteval Arg<type> otherwise(T value) const noexcept {
    Arg<type> arg(*this, value, this->set_value);
    arg.action_fn = actions::assign<T>;
    arg.is_required = false;
    return arg;
  }

  consteval Arg<type> otherwise(char const *value) const noexcept { return otherwise(std::string_view(value)); }

  consteval Arg<type> required() const noexcept {
    auto arg = *this;
    arg.is_required = true;
    return arg;
  }

  template <typename T>
  consteval Arg<type> set(T value) const noexcept requires(type != ArgumentType::POSITIONAL) {
    Arg<type> arg(*this, this->default_value, value);
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  consteval Arg<type> set(char const *value) const noexcept requires(type != ArgumentType::POSITIONAL) {
    return set(std::string_view(value));
  }

  bool has_abbrev() const noexcept requires(type != ArgumentType::POSITIONAL) { return !abbrev.empty(); }
  bool has_default() const noexcept { return default_value.index() != 0; }
  bool has_set() const noexcept { return set_value.index() != 0; }

  void set_default_to(ArgValue &arg) const noexcept requires(type != ArgumentType::FLAG) {
    if (default_setter != nullptr) {
      default_setter(arg);
    } else if (has_default()) {
      ArgValueSetter setter(arg);
      std::visit(setter, default_value);
    }
  }

  void set_default_to(ArgValue &arg) const noexcept requires(type == ArgumentType::FLAG) {
    ArgValueSetter setter(arg);
    std::visit(setter, default_value);
  }

  std::string format_base_usage() const noexcept;
  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;

private:
  // workaround for not being able to change the value of a variant after it's been constructed
  template <typename Default, typename Set>
  consteval Arg(Arg<type> const &other, Default d, Set s)
      : name(other.name), abbrev(other.abbrev), description(other.description), is_required(other.is_required),
        default_value(d), set_value(s), action_fn(other.action_fn), gather_info(other.gather_info),
        default_setter(other.default_setter) {}
};

constexpr auto Help = Flg("help", "h").help("Display this information").action(actions::print_help);
constexpr auto Version = Flg("version", "V").help("Display the software version").action(actions::print_version);

template <ArgumentType type>
bool operator<(Arg<type> const &lhs, Arg<type> const &rhs) noexcept {
  if (lhs.is_required == rhs.is_required)
    return lhs.name < rhs.name;
  return lhs.is_required && !rhs.is_required;
}

class Cmd {
public:
  // just a helper to encapsulate the pointer indirection
  Program *program;

  Cmd(Program &program) : program(&program) {}

  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;

  auto operator<=>(Cmd const &) const noexcept;

private:
};

struct ParsedOption {
  std::string_view name;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

ParsedOption parse_option(std::string_view const) noexcept;

class Program {
public:
  std::string_view name{};
  std::string_view version{};
  std::string_view title{};
  std::string_view introduction{};
  std::string_view description{};

  std::size_t msg_width = 100;
  opzioni::error_handler error_handler = print_error;
  bool has_auto_help{false};

  Program() = default;
  Program(std::string_view name) : Program(name, {}) {}
  Program(std::string_view name, std::string_view title) : name(name), title(title) {}

  Program &intro(std::string_view) noexcept;
  Program &details(std::string_view) noexcept;
  Program &v(std::string_view) noexcept; // I don't know what else to call this

  Program &max_width(std::size_t) noexcept;
  Program &on_error(opzioni::error_handler) noexcept;

  ArgMap operator()(int, char const *[]) const;
  ArgMap operator()(std::span<char const *>) const;

  template <typename T>
  Program &operator+(T arg) {
    return add(arg);
  }

  std::vector<Cmd> const &cmds() const noexcept { return _cmds; }
  std::vector<Flg> const &flags() const noexcept { return _flags; }
  std::vector<Opt> const &options() const noexcept { return _options; }
  std::vector<Pos> const &positionals() const noexcept { return _positionals; }

  auto find_cmd(std::string_view name) const noexcept {
    return std::ranges::find(_cmds, name, [](auto const &cmd) { return cmd.program->name; });
  }

  bool has_cmd(std::string_view name) const noexcept { return find_cmd(name) != _cmds.end(); }

  auto find_flg(std::string_view name) const noexcept {
    return std::ranges::find_if(_flags, [name](auto const &flg) { return flg.name == name || flg.abbrev == name; });
  }

  bool has_flg(std::string_view name) const noexcept { return find_flg(name) != _flags.end(); }

  auto find_opt(std::string_view name) const noexcept {
    return std::ranges::find_if(_options, [name](auto const &opt) { return opt.name == name || opt.abbrev == name; });
  }

  bool has_opt(std::string_view name) const noexcept { return find_opt(name) != _options.end(); }

  auto find_pos(std::string_view name) const noexcept {
    return std::ranges::find(_positionals, name, [](auto const &pos) { return pos.name; });
  }

  bool has_pos(std::string_view name) const noexcept { return find_pos(name) != _positionals.end(); }

private:
  std::vector<Cmd> _cmds;
  std::vector<Flg> _flags;
  std::vector<Opt> _options;
  std::vector<Pos> _positionals;

  Program &add(Cmd);
  Program &add(Flg);
  Program &add(Opt);
  Program &add(Pos);

  ArgMap parse(std::span<char const *>) const;
  void check_contains_required(ArgMap const &) const;
  void set_defaults(ArgMap &) const noexcept;

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
  std::string const program_name;
  std::string const program_version;
  std::string const program_title;
  std::string const program_introduction;
  std::string const program_description;
  std::vector<Cmd> cmds;
  std::vector<Flg> flags;
  std::vector<Opt> options;
  std::vector<Pos> positionals;

  void print_arg_help(auto const &arg, std::string_view const padding) const noexcept {
    using std::views::drop;
    auto const description = arg.format_for_help_description();
    // -8 because we'll later print a left margin of 8 spaces (4 of indentation, 4 of alignment)
    auto const description_lines = limit_within(description, max_width - padding.size() - 8);
    // -4 again because we'll shift it 4 spaces into the padding, invading it,
    // so we can have the possible following lines indented
    out << fmt::format("    {:<{}} {}\n", arg.format_for_help_index(), padding.size() - 4,
                       fmt::join(description_lines.front(), " "));
    for (auto const &line : description_lines | drop(1)) {
      out << fmt::format("    {} {}\n", padding, fmt::join(line, " "));
    };
  }
};

// +---------------------------+
// | implementation of actions |
// +---------------------------+

namespace actions {

// +--------+
// | assign |
// +--------+

template <typename T>
void assign_to(ArgMap &map, std::string_view const name, T value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted)
    throw DuplicateAssignment(name);
}

template <typename T>
void assign(Program const &, ArgMap &map, Flg const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, std::get<T>(arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Opt const &arg, std::optional<std::string_view> const parsed_value) {
  if (parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(arg.set_value));
}

template <typename T>
void assign(Program const &, ArgMap &map, Pos const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<T>(*parsed_value));
}

// +--------+
// | append |
// +--------+

template <typename Elem>
void append_to(ArgMap &map, std::string_view const name, Elem value) {
  if (auto list = map.args.find(name); list != map.args.end()) {
    std::get<std::vector<Elem>>(list->second.value).emplace_back(std::move(value));
  } else {
    assign_to(map, name, std::vector<Elem>{std::move(value)});
  }
}

template <typename Elem>
void append(Program const &, ArgMap &map, Flg const &arg, std::optional<std::string_view> const parsed_value) {
  append_to<Elem>(map, arg.name, std::get<Elem>(arg.set_value));
}

template <typename Elem>
void append(Program const &, ArgMap &map, Opt const &arg, std::optional<std::string_view> const parsed_value) {
  if (parsed_value)
    append_to<Elem>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem>(map, arg.name, std::get<Elem>(arg.set_value));
}

template <typename Elem>
void append(Program const &, ArgMap &map, Pos const &arg, std::optional<std::string_view> const parsed_value) {
  append_to<Elem>(map, arg.name, convert<Elem>(*parsed_value));
}

// +-----+
// | csv |
// +-----+

template <typename T>
void csv(Program const &, ArgMap &map, Opt const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<std::vector<T>>(*parsed_value));
}

template <typename T>
void csv(Program const &, ArgMap &map, Pos const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<std::vector<T>>(*parsed_value));
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_H
