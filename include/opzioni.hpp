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
using BuiltinTypes = TypeList<std::monostate, bool, int, double, std::string_view>;
using BuiltinVariant = VariantOf<BuiltinTypes>::type;

// types that may be the result of parsing the CLI
using ExternalTypes = Concat<BuiltinTypes, VectorOf<BuiltinTypes>::type>::type;
using ExternalVariant = VariantOf<ExternalTypes>::type;

std::string builtin2str(BuiltinVariant const &) noexcept;

// +----------------------+
// | forward declarations |
// +----------------------+
enum struct ArgType { POS, OPT, FLG };

struct Arg;
struct ArgMap;
class Program;

consteval void validate_arg(Arg const &) noexcept;

template <std::size_t N>
consteval void validate_args(std::array<Arg, N> const &, Arg const &) noexcept;

// +----------------+
// | error handlers |
// +----------------+
using ErrorHandler = int (*)(Program const &, UserError const &);

int print_error(Program const &, UserError const &) noexcept;
int print_error_and_usage(Program const &, UserError const &) noexcept;
int rethrow(Program const &, UserError const &);

// +---------+
// | actions |
// +---------+
namespace actions {

using Signature = void (*)(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <typename T>
void assign(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <typename Elem>
void append(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <typename T>
void csv(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_help(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_version(Program const &, ArgMap &, Arg const &, std::optional<std::string_view> const);

} // namespace actions

// +-----------+
// | arguments |
// +-----------+

struct ArgValue {
  ExternalVariant value{};

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

using DefaultValueSetter = void (*)(ArgValue &);

template <typename T>
void set_empty_vector(ArgValue &arg) noexcept {
  arg.value = std::vector<T>{};
}

// +-----+
// | Arg |
// +-----+

struct Arg {
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view description{};
  bool is_required = false;
  BuiltinVariant default_value{};
  BuiltinVariant set_value{};
  actions::Signature action_fn = actions::assign<std::string_view>;
  std::size_t gather_amount = 1;
  DefaultValueSetter default_setter = nullptr;

  consteval Arg action(actions::Signature action_fn) const noexcept {
    auto arg = *this;
    arg.action_fn = action_fn;
    return arg;
  }

  template <typename T>
  consteval Arg append() const noexcept {
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
    arg.is_required = false;
    arg.action_fn = actions::append<T>;
    arg.default_setter = set_empty_vector<T>;
    return arg;
  }

  template <typename T>
  consteval Arg csv_of() const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use the csv action because they do not take values from the command-line";
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
    arg.is_required = false;
    arg.action_fn = actions::csv<T>;
    arg.default_setter = set_empty_vector<T>;
    return arg;
  }

  template <typename T = std::string_view>
  consteval Arg gather(std::size_t amount) const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use gather because they do not take values from the command-line";
    if (amount != 1) {
      auto arg = Arg::With(*this, std::monostate{}, this->set_value);
      arg.is_required = false;
      arg.gather_amount = amount;
      arg.action_fn = actions::append<T>;
      arg.default_setter = set_empty_vector<T>;
      return arg;
    }
    auto arg = *this;
    arg.gather_amount = amount;
    arg.action_fn = actions::append<T>;
    return arg;
  }

  template <typename T = std::string_view>
  consteval Arg gather() const noexcept {
    return gather<T>(0);
  }

  consteval Arg help(std::string_view description) const noexcept {
    auto arg = *this;
    arg.description = description;
    return arg;
  }

  template <typename T>
  consteval Arg of() const noexcept {
    auto arg = *this;
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  template <typename T>
  consteval Arg otherwise(T value) const noexcept {
    auto arg = Arg::With(*this, value, this->set_value);
    arg.action_fn = actions::assign<T>;
    arg.is_required = false;
    return arg;
  }

  consteval Arg otherwise(char const *value) const noexcept { return otherwise(std::string_view(value)); }

  consteval Arg otherwise(DefaultValueSetter setter) const noexcept {
    auto arg = *this;
    arg.default_setter = setter;
    arg.is_required = false;
    return arg;
  }

  consteval Arg required() const noexcept {
    if (this->has_default())
      throw "A required argument cannot have a default value";
    auto arg = *this;
    arg.is_required = true;
    return arg;
  }

  template <typename T>
  consteval Arg set(T value) const noexcept {
    if (this->type == ArgType::POS)
      throw "Positionals cannot use set value because they always take a value from the command-line";
    auto arg = Arg::With(*this, this->default_value, value);
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  consteval Arg set(char const *value) const noexcept { return set(std::string_view(value)); }

  constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  constexpr bool has_default() const noexcept { return default_value.index() != 0 || default_setter != nullptr; }
  constexpr bool has_set() const noexcept { return set_value.index() != 0; }

  void set_default_to(ArgValue &arg) const noexcept {
    if (default_setter != nullptr) {
      default_setter(arg);
    } else if (default_value.index() != 0) {
      ArgValueSetter setter(arg);
      std::visit(setter, default_value);
    }
  }

  std::string format_base_usage() const noexcept;
  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;

  // workaround for not being able to change the value of a variant after it's been constructed
  template <typename Default, typename Set>
  static consteval Arg With(Arg const &other, Default d, Set s) noexcept {
    return Arg{.type = other.type,
               .name = other.name,
               .abbrev = other.abbrev,
               .description = other.description,
               .is_required = other.is_required,
               .default_value = d,
               .set_value = s,
               .action_fn = other.action_fn,
               .gather_amount = other.gather_amount,
               .default_setter = other.default_setter};
  }
};

constexpr bool operator<(Arg const &lhs, Arg const &rhs) noexcept { return lhs.name < rhs.name; }

constexpr bool operator==(Arg const &lhs, Arg const &rhs) noexcept {
  auto const same_name = lhs.name == rhs.name;
  auto const same_abbrev = lhs.has_abbrev() && rhs.has_abbrev() && lhs.abbrev == rhs.abbrev;
  return same_name || same_abbrev;
}

consteval auto operator*(Arg const lhs, Arg const rhs) noexcept {
  if (lhs == rhs)
    throw "Trying to add argument with a duplicate name";
  validate_arg(lhs);
  validate_arg(rhs);
  return std::array<Arg, 2>{lhs, rhs};
}

template <std::size_t N>
consteval auto operator*(std::array<Arg, N> const args, Arg const other) noexcept {
  validate_arg(other);
  validate_args(args, other);

  std::array<Arg, N + 1> newargs;
  std::copy_n(args.begin(), N, newargs.begin());
  newargs[N] = other;
  return newargs;
}

consteval void validate_arg(Arg const &arg) noexcept {
  if (arg.type == ArgType::POS && arg.has_abbrev())
    throw "Positionals cannot have abbreviations";

  if (arg.type == ArgType::POS && arg.has_set())
    throw "Positionals cannot use set value because they always take a value from the command-line";

  if (arg.type == ArgType::FLG && arg.gather_amount != 1) // 1 is the default
    throw "Flags cannot use gather because they do not take values from the command-line";

  if (arg.has_abbrev() && arg.abbrev.length() != 1)
    throw "Abbreviations must be a single letter";

  if (arg.is_required && arg.has_default())
    throw "A required argument cannot have a default value";

  if (arg.default_value.index() != 0 && arg.set_value.index() != 0 &&
      arg.default_value.index() != arg.set_value.index())
    throw "The default and set values must be of the same type";
}

template <std::size_t N>
consteval void validate_args(std::array<Arg, N> const &args, Arg const &other) noexcept {
  if (std::ranges::find(args, other) != args.end())
    throw "Trying to add argument with a duplicate name";
}

// +----------------------+
// | Arg creation helpers |
// +----------------------+

consteval Arg Flg(std::string_view name, std::string_view abbrev) noexcept {
  if (!abbrev.empty() && abbrev.length() != 1)
    throw "Abbreviations must be a single letter";
  return Arg{.type = ArgType::FLG,
             .name = name,
             .abbrev = abbrev,
             .default_value = false,
             .set_value = true,
             .action_fn = actions::assign<bool>};
}

consteval Arg Flg(std::string_view name) noexcept { return Flg(name, {}); }

consteval Arg Opt(std::string_view name, std::string_view abbrev) noexcept {
  if (!abbrev.empty() && abbrev.length() != 1)
    throw "Abbreviations must be a single letter";
  return Arg{.type = ArgType::OPT, .name = name, .abbrev = abbrev};
}

consteval Arg Opt(std::string_view name) noexcept { return Opt(name, {}); }

consteval Arg Pos(std::string_view name) noexcept {
  return Arg{.type = ArgType::POS, .name = name, .is_required = true};
}

consteval Arg Help(std::string_view description) noexcept {
  return Flg("help", "h").help(description).action(actions::print_help);
}

consteval Arg Help() noexcept { return Help("Display this information"); }

consteval Arg Version(std::string_view description) noexcept {
  return Flg("version", "V").help(description).action(actions::print_version);
}

consteval Arg Version() noexcept { return Version("Display the software version"); }

class Cmd {
public:
  // just a helper to encapsulate the pointer indirection
  Program const *program;

  Cmd(Program const &program) : program(&program) {}

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

// +---------+
// | Program |
// +---------+

class Program {
public:
  std::string_view name{};
  std::string_view version{};
  std::string_view title{};
  std::string_view introduction{};
  std::string_view description{};

  std::size_t msg_width = 100;
  ErrorHandler error_handler = print_error;
  // bool has_auto_help{false};
  std::size_t positionals_amount = 0;

  Program() = default;
  Program(std::string_view name) : Program(name, {}) {}
  Program(std::string_view name, std::string_view title) : name(name), title(title) {}

  Program &intro(std::string_view) noexcept;
  Program &details(std::string_view) noexcept;
  Program &v(std::string_view) noexcept; // I don't know what else to call this

  Program &max_width(std::size_t) noexcept;
  Program &on_error(ErrorHandler) noexcept;

  Program &operator+(Cmd cmd) { return add(cmd); }

  template <std::size_t N>
  Program &operator+(std::array<Arg, N> args) {
    _args.reserve(N);
    for (auto &&arg : args)
      add(arg);

    // putting all positionals first without changing their relative order, then sorting all other args.
    // We wouldn't need the following sort, but then it's already done if we end up printing the help text.
    std::ranges::stable_partition(_args, [](auto const &arg) { return arg.type == ArgType::POS; });
    std::stable_sort(std::next(_args.begin(), positionals_amount), _args.end());
    return *this;
  }

  ArgMap operator()(int, char const *[]) const;
  ArgMap operator()(std::span<char const *>) const;

  std::vector<Arg> const &args() const noexcept { return _args; }
  std::vector<Cmd> const &cmds() const noexcept { return _cmds; }

  constexpr auto find_arg(std::string_view name, ArgType type) const noexcept {
    return std::ranges::find_if(_args, [name, type](auto const &arg) {
      return arg.type == type && (arg.name == name || (arg.has_abbrev() && arg.abbrev == name));
    });
  }

  bool has_arg(std::string_view name, ArgType type) const noexcept { return find_arg(name, type) != _args.end(); }
  bool has_flg(std::string_view name) const noexcept { return has_arg(name, ArgType::FLG); }
  bool has_opt(std::string_view name) const noexcept { return has_arg(name, ArgType::OPT); }
  bool has_pos(std::string_view name) const noexcept { return has_arg(name, ArgType::POS); }

  constexpr auto find_cmd(std::string_view name) const noexcept {
    return std::ranges::find(_cmds, name, [](auto const &cmd) { return cmd.program->name; });
  }

  bool has_cmd(std::string_view name) const noexcept { return find_cmd(name) != _cmds.end(); }

private:
  std::vector<Arg> _args;
  std::vector<Cmd> _cmds;

  Program &add(Arg);
  Program &add(Cmd);

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
  std::size_t positionals_amount;
  std::vector<Cmd> cmds;
  std::vector<Arg> args;

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
void assign(Program const &, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  if (arg.type != ArgType::FLG && parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(arg.set_value));
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
void append(Program const &, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  if (arg.type != ArgType::FLG && parsed_value)
    append_to<Elem>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem>(map, arg.name, std::get<Elem>(arg.set_value));
}

// +-----+
// | csv |
// +-----+

template <typename T>
void csv(Program const &, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<std::vector<T>>(*parsed_value));
}

} // namespace actions

} // namespace opzioni

#endif // OPZIONI_H
