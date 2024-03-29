#ifndef OPZIONI_H
#define OPZIONI_H

#include "converters.hpp"
#include "exceptions.hpp"
#include "string.hpp"
#include "types.hpp"

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

std::string builtin2str(BuiltinVariant const &) noexcept;

// +----------------------+
// | forward declarations |
// +----------------------+

struct Arg;
struct ArgMap;
class ProgramView;

template <std::size_t ArgsSize = 0, std::size_t CmdsSize = 0>
class Program;

consteval void validate_arg(Arg const &) noexcept;

template <std::size_t N>
consteval void validate_args(std::array<Arg, N> const &, Arg const &) noexcept;

template <std::size_t N>
consteval void validate_cmds(std::array<ProgramView, N> const &, ProgramView const &) noexcept;

// +----------------+
// | error handlers |
// +----------------+
using ErrorHandler = int (*)(ProgramView const, UserError const &);

int print_error(ProgramView const, UserError const &) noexcept;
int print_error_and_usage(ProgramView const, UserError const &) noexcept;
int rethrow(ProgramView const, UserError const &);

// +--------------+
// | argument map |
// +--------------+

struct ArgValue {
  ExternalVariant value{};

  template <concepts::ExternalType T>
  T as() const {
    return std::get<T>(value);
  }

  template <concepts::ExternalType T>
  operator T() const {
    return as<T>();
  }
};

class ArgValueSetter {
public:
  ArgValueSetter(ArgValue &arg) : arg(arg) {}

  template <concepts::ExternalType T>
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

  template <concepts::ExternalType T>
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

// +-------------------------------+
// | generic default value setters |
// +-------------------------------+

using DefaultValueSetter = void (*)(ArgValue &);

template <concepts::BuiltinType T>
void set_empty_vector(ArgValue &arg) noexcept {
  arg.value = std::vector<T>{};
}

// +------------------+
// | action functions |
// +------------------+

namespace act::fn {

using Signature = void (*)(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType T>
void assign(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType Elem>
void append(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

void count(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType Elem>
void csv(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_help(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_version(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

} // namespace act::fn

namespace concepts {

// unfortunately, this concept has to be here instead of in types.hpp
// because it references stuff defined in this header :(
// (we would have a circular dependency)
template <typename A>
concept Action = requires(A action) {
  typename A::value_type;
  requires BuiltinType<typename A::value_type>;

  { action.get_is_required() }
  noexcept->std::same_as<bool>;

  { action.get_default_value() }
  noexcept->std::same_as<std::optional<typename A::value_type>>;

  { action.get_implicit_value() }
  noexcept->std::same_as<std::optional<typename A::value_type>>;

  { action.get_fn() }
  noexcept->std::same_as<opzioni::act::fn::Signature>;
  { action.get_gather_amount() }
  noexcept->std::same_as<std::size_t>;

  { action.get_default_setter() }
  noexcept->std::same_as<opzioni::DefaultValueSetter>;
};

} // namespace concepts

// +---------+
// | actions |
// +---------+

template <concepts::BuiltinType Elem = std::string_view>
class Append {
public:
  using value_type = Elem;

  consteval Append() = default;

  consteval Append<Elem> gather(std::size_t amount) const noexcept {
    return Append(this->is_required, this->implicit_value, amount, this->default_setter);
  }

  consteval Append<Elem> gather() const noexcept { return gather(0); }

  consteval Append<Elem> implicitly(Elem value) const noexcept {
    return Append(this->is_required, value, this->gather_amount, this->default_setter);
  }

  consteval Append<Elem> implicitly(char const *value) const noexcept requires std::is_same_v<Elem, std::string_view> {
    return implicitly(std::string_view(value));
  }

  consteval Append<Elem> otherwise(DefaultValueSetter setter) const noexcept {
    return Append(false, this->implicit_value, this->gather_amount, setter);
  }

  consteval Append<Elem> required() const noexcept {
    return Append(true, this->implicit_value, this->gather_amount, nullptr);
  }

  consteval bool get_is_required() const noexcept { return this->is_required; }
  consteval std::optional<Elem> get_default_value() const noexcept { return std::nullopt; }
  consteval std::optional<Elem> get_implicit_value() const noexcept { return this->implicit_value; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::append<Elem>; }
  consteval std::size_t get_gather_amount() const noexcept { return this->gather_amount; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return this->default_setter; }

private:
  bool is_required = false;
  std::optional<Elem> implicit_value{};
  std::size_t gather_amount = 1;
  DefaultValueSetter default_setter = set_empty_vector<Elem>;

  consteval Append(bool is_required, std::optional<Elem> implicit_value, std::size_t gather_amount,
                   DefaultValueSetter default_setter)
      : is_required(is_required), implicit_value(implicit_value), gather_amount(gather_amount),
        default_setter(default_setter) {}
};

template <concepts::BuiltinType Elem = std::string_view>
class Assign {
public:
  using value_type = Elem;

  consteval Assign() = default;

  consteval Assign<Elem> implicitly(Elem value) const noexcept {
    return Assign(this->is_required, this->default_value, value);
  }

  consteval Assign<Elem> implicitly(char const *value) const noexcept requires std::is_same_v<Elem, std::string_view> {
    return implicitly(std::string_view(value));
  }

  consteval Assign<Elem> optional() const noexcept { return Assign(false, this->default_value, this->implicit_value); }

  consteval Assign<Elem> otherwise(Elem value) const noexcept { return Assign(false, value, this->implicit_value); }

  consteval Assign<Elem> otherwise(char const *value) const noexcept requires std::is_same_v<Elem, std::string_view> {
    return otherwise(std::string_view(value));
  }

  consteval Assign<Elem> required() const noexcept { return Assign(true, std::nullopt, this->implicit_value); }

  consteval bool get_is_required() const noexcept { return this->is_required; }
  consteval std::optional<Elem> get_default_value() const noexcept { return this->default_value; }
  consteval std::optional<Elem> get_implicit_value() const noexcept { return this->implicit_value; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::assign<Elem>; }
  consteval std::size_t get_gather_amount() const noexcept { return 1; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return nullptr; }

private:
  bool is_required = true;
  std::optional<Elem> default_value{};
  std::optional<Elem> implicit_value{};

  consteval Assign(bool is_required, std::optional<Elem> default_value, std::optional<Elem> implicit_value)
      : is_required(is_required), default_value(default_value), implicit_value(implicit_value) {}
};

class Count {
public:
  using value_type = std::size_t;

  consteval bool get_is_required() const noexcept { return false; }
  consteval std::optional<std::size_t> get_default_value() const noexcept { return std::size_t{0}; }
  consteval std::optional<std::size_t> get_implicit_value() const noexcept { return std::nullopt; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::count; }
  consteval std::size_t get_gather_amount() const noexcept { return 1; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return nullptr; }
};

template <concepts::BuiltinType Elem = std::string_view>
class List {
public:
  using value_type = Elem;

  consteval List<Elem> otherwise(DefaultValueSetter setter) const noexcept {
    auto list = *this;
    list.is_required = false;
    list.default_setter = setter;
    return list;
  }

  consteval List<Elem> required() const noexcept {
    auto list = *this;
    list.is_required = true;
    list.default_setter = nullptr;
    return list;
  }

  consteval bool get_is_required() const noexcept { return this->is_required; }
  consteval std::optional<Elem> get_default_value() const noexcept { return std::nullopt; }
  consteval std::optional<Elem> get_implicit_value() const noexcept { return std::nullopt; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::csv<Elem>; }
  consteval std::size_t get_gather_amount() const noexcept { return 1; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return this->default_setter; }

private:
  bool is_required = false;
  DefaultValueSetter default_setter = set_empty_vector<Elem>;
};

class PrintHelp {
public:
  using value_type = bool;

  consteval bool get_is_required() const noexcept { return false; }
  consteval std::optional<bool> get_default_value() const noexcept { return false; }
  consteval std::optional<bool> get_implicit_value() const noexcept { return true; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::print_help; }
  consteval std::size_t get_gather_amount() const noexcept { return 1; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return nullptr; }
};

class PrintVersion {
public:
  using value_type = bool;

  consteval bool get_is_required() const noexcept { return false; }
  consteval std::optional<bool> get_default_value() const noexcept { return false; }
  consteval std::optional<bool> get_implicit_value() const noexcept { return true; }
  consteval act::fn::Signature get_fn() const noexcept { return act::fn::print_version; }
  consteval std::size_t get_gather_amount() const noexcept { return 1; }
  consteval DefaultValueSetter get_default_setter() const noexcept { return nullptr; }
};

// +-----+
// | Arg |
// +-----+

enum struct ArgType { POS, OPT, FLG };

struct Arg {
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view description{};
  bool is_required = true;
  BuiltinVariant default_value{};
  BuiltinVariant implicit_value{};
  act::fn::Signature action_fn = act::fn::assign<std::string_view>;
  std::size_t gather_amount = 1;
  DefaultValueSetter default_setter = nullptr;

  consteval Arg help(std::string_view description) const noexcept {
    auto arg = *this;
    arg.description = description;
    return arg;
  }

  template <concepts::Action Action>
  consteval Arg action(Action action) const noexcept {
    auto const &default_value = action.get_default_value();
    auto const &implicit_value = action.get_implicit_value();
    auto arg = default_value && implicit_value
                   ? Arg::With(*this, *default_value, *implicit_value)
                   : default_value ? Arg::With(*this, *default_value, std::monostate{})
                                   : implicit_value ? Arg::With(*this, std::monostate{}, *implicit_value)
                                                    : Arg::With(*this, std::monostate{}, std::monostate{});
    arg.is_required = action.get_is_required();
    arg.action_fn = action.get_fn();
    arg.gather_amount = action.get_gather_amount();
    arg.default_setter = action.get_default_setter();
    return arg;
  }

  constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  constexpr bool has_default() const noexcept { return default_value.index() != 0 || default_setter != nullptr; }
  constexpr bool has_implicit() const noexcept { return implicit_value.index() != 0; }
  constexpr bool is_positional() const noexcept { return type == ArgType::POS; }

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
  static consteval Arg With(Arg const &other, BuiltinVariant default_value, BuiltinVariant implicit_value) noexcept {
    return Arg{.type = other.type,
               .name = other.name,
               .abbrev = other.abbrev,
               .description = other.description,
               .is_required = other.is_required,
               .default_value = default_value,
               .implicit_value = implicit_value,
               .action_fn = other.action_fn,
               .gather_amount = other.gather_amount,
               .default_setter = other.default_setter};
  }
};

constexpr bool operator<(Arg const &lhs, Arg const &rhs) noexcept {
  bool const lhs_is_positional = lhs.type == ArgType::POS;
  bool const rhs_is_positional = rhs.type == ArgType::POS;

  if (lhs_is_positional && rhs_is_positional)
    return false; // don't move positionals relative to each other

  if (!lhs_is_positional && !rhs_is_positional)
    return lhs.name < rhs.name; // sort non-positionals by name

  return lhs_is_positional; // sort positionals before other types
}

constexpr bool operator==(Arg const &lhs, Arg const &rhs) noexcept {
  auto const same_name = lhs.name == rhs.name;
  auto const same_abbrev = lhs.has_abbrev() && rhs.has_abbrev() && lhs.abbrev == rhs.abbrev;
  return same_name || same_abbrev;
}

consteval void validate_arg(Arg const &arg) noexcept {
  if (arg.name.empty())
    throw "An argument cannot have an empty name";

  if (arg.type == ArgType::POS && arg.has_abbrev())
    throw "Positionals cannot have abbreviations";

  if (arg.has_abbrev() && arg.abbrev.length() != 1)
    throw "Abbreviations must be a single character";

  if (!is_valid_name(arg.name))
    throw "Argument names can only contain alphanumeric characters and - or _,"
          "and must begin with a letter and end with a letter or a number";

  if (!is_valid_abbrev(arg.abbrev))
    throw "Argument abbreviations can only contain alphanumeric characters";

  if (arg.type == ArgType::POS && arg.has_implicit())
    throw "Positionals cannot use implicit value because they always take a value from the command-line";

  if (arg.type == ArgType::FLG && arg.gather_amount != 1) // 1 is the default
    throw "Flags cannot use gather because they do not take values from the command-line";

  if (arg.is_required && arg.has_default())
    throw "A required argument cannot have a default value";

  if (!arg.is_required && !arg.has_default())
    throw "An optional argument must have a default value";

  if (arg.default_value.index() != 0 && arg.implicit_value.index() != 0 &&
      arg.default_value.index() != arg.implicit_value.index())
    throw "The default and implicit values must be of the same type";

  if (arg.default_value.index() != 0 && arg.default_setter != nullptr)
    throw "An argument cannot have a default value and a default value setter at the same time";
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
  auto const arg = Arg{.type = ArgType::FLG,
                       .name = name,
                       .abbrev = abbrev,
                       .is_required = false,
                       .default_value = false,
                       .implicit_value = true,
                       .action_fn = act::fn::assign<bool>};
  validate_arg(arg);
  return arg;
}

consteval Arg Flg(std::string_view name) noexcept { return Flg(name, {}); }

consteval Arg Opt(std::string_view name, std::string_view abbrev) noexcept {
  auto const arg = Arg{.type = ArgType::OPT, .name = name, .abbrev = abbrev, .is_required = false, .default_value = ""};
  validate_arg(arg);
  return arg;
}

consteval Arg Opt(std::string_view name) noexcept { return Opt(name, {}); }

consteval Arg Pos(std::string_view name) noexcept {
  auto const arg = Arg{.type = ArgType::POS, .name = name, .is_required = true};
  validate_arg(arg);
  return arg;
}

// +--------------------+
// | argument shortcuts |
// +--------------------+

consteval Arg Counter(std::string_view name, std::string_view abbrev) noexcept {
  return Flg(name, abbrev).action(Count());
}

consteval Arg Counter(std::string_view name) noexcept { return Counter(name, {}); }

consteval Arg Help(std::string_view description) noexcept {
  return Flg("help", "h").help(description).action(PrintHelp());
}

consteval Arg Help() noexcept { return Help("Display this information"); }

consteval Arg Version(std::string_view description) noexcept {
  return Flg("version", "V").help(description).action(PrintVersion());
}

consteval Arg Version() noexcept { return Version("Display the software version"); }

// +--------------------+
// | arg and cmd search |
// +--------------------+

constexpr auto find_arg(ProgramView const program, std::string_view name, ArgType type) noexcept;
constexpr bool has_arg(ProgramView const program, std::string_view name, ArgType type) noexcept;
constexpr bool has_flg(ProgramView const program, std::string_view name) noexcept;
constexpr bool has_opt(ProgramView const program, std::string_view name) noexcept;
constexpr bool has_pos(ProgramView const program, std::string_view name) noexcept;
constexpr auto find_cmd(ProgramView const program, std::string_view name) noexcept;
constexpr bool has_cmd(ProgramView const program, std::string_view name) noexcept;

// +---------+
// | parsing |
// +---------+

ArgMap parse(ProgramView const program, std::span<char const *> args);

// +-----------------+
// | ProgramMetadata |
// +-----------------+

struct ProgramMetadata {
  std::string_view name{};
  std::string_view title{};
  std::string_view version{};
  std::string_view introduction{};
  std::string_view details{};

  std::size_t msg_width = 100;
  ErrorHandler error_handler = print_error_and_usage;

  std::size_t positionals_amount = 0;

  constexpr auto operator<=>(ProgramMetadata const &) const noexcept = default;
};

// +-------------+
// | ProgramView |
// +-------------+

class ProgramView {
public:
  ProgramMetadata metadata{};

  constexpr ProgramView() = default;

  constexpr ProgramView(ProgramMetadata metadata, Arg const *args_begin, std::size_t args_size,
                        ProgramView const *cmds_begin, std::size_t cmds_size)
      : metadata(metadata), args_begin(args_begin), args_size(args_size), cmds_begin(cmds_begin), cmds_size(cmds_size) {
  }

  constexpr auto args() const noexcept { return std::span<Arg const>(args_begin, args_size); }
  constexpr auto cmds() const noexcept { return std::span<ProgramView const>(cmds_begin, cmds_size); }

  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;

private:
  Arg const *args_begin = nullptr;
  std::size_t args_size = 0;
  ProgramView const *cmds_begin = nullptr;
  std::size_t cmds_size = 0;
};

constexpr bool operator<(ProgramView const &lhs, ProgramView const &rhs) noexcept {
  return lhs.metadata.name < rhs.metadata.name;
}

constexpr bool operator==(ProgramView const &lhs, ProgramView const &rhs) noexcept {
  return lhs.metadata.name == rhs.metadata.name;
}

template <std::size_t N>
consteval void validate_cmds(std::array<ProgramView, N> const &cmds, ProgramView const &other) noexcept {
  if (std::ranges::find(cmds, other) != cmds.end())
    throw "Trying to add command with a duplicate name";
}

// +---------+
// | Program |
// +---------+

template <std::size_t ArgsSize, std::size_t CmdsSize>
class Program {
public:
  ProgramMetadata metadata{};
  std::array<Arg, ArgsSize> args;
  std::array<ProgramView, CmdsSize> cmds;

  consteval Program(std::string_view name) : Program(name, {}) {}
  consteval Program(std::string_view name, std::string_view title) : metadata{.name = name, .title = title} {
    if (!is_valid_name(name))
      throw "Program names cannot be empty, can only contain alphanumeric characters and - or _,"
            "and must begin with a letter and end with a letter or a number";
  }

  template <std::size_t OtherArgsSize, std::size_t OtherCmdsSize>
  consteval Program(Program<OtherArgsSize, OtherCmdsSize> const &other) : metadata(other.metadata) {
    static_assert(ArgsSize >= OtherArgsSize,
                  "attempting to copy-construct a Program from another that has more args than the new one could hold");
    static_assert(CmdsSize >= OtherCmdsSize,
                  "attempting to copy-construct a Program from another that has more cmds than the new one could hold");
    std::copy_n(other.args.begin(), OtherArgsSize, args.begin()); // don't know why std::ranges::copy_n doesn't work
    std::copy_n(other.cmds.begin(), OtherCmdsSize, cmds.begin()); // don't know why std::ranges::copy_n doesn't work
  }

  consteval auto intro(std::string_view introduction) const noexcept {
    auto program = *this;
    program.metadata.introduction = introduction;
    return program;
  }

  consteval auto details(std::string_view details) const noexcept {
    auto program = *this;
    program.metadata.details = details;
    return program;
  }

  consteval auto version(std::string_view version) const noexcept {
    auto program = *this;
    program.metadata.version = version;
    return program;
  }

  consteval auto msg_width(std::size_t msg_width) const noexcept {
    auto program = *this;
    program.metadata.msg_width = msg_width;
    return program;
  }

  consteval auto on_error(ErrorHandler error_handler) const noexcept {
    if (error_handler == nullptr)
      throw "A program or command must have an error handler. If you don't want the"
            "default try... catch behavior, just use parse(ProgramView const, std::span<char const *>)";
    auto program = *this;
    program.metadata.error_handler = error_handler;
    return program;
  }

  consteval Program<ArgsSize + 1, CmdsSize> add(Arg const arg) const noexcept {
    validate_arg(arg);
    validate_args(this->args, arg);
    Program<ArgsSize + 1, CmdsSize> new_program(*this);
    new_program.args[ArgsSize] = arg;
    new_program.metadata.positionals_amount += arg.type == ArgType::POS;
    std::sort(new_program.args.begin(), new_program.args.end());
    return new_program;
  }

  consteval Program<ArgsSize, CmdsSize + 1> add(ProgramView const cmd) const noexcept {
    validate_cmds(this->cmds, cmd);
    Program<ArgsSize, CmdsSize + 1> new_program(*this);
    new_program.cmds[CmdsSize] = cmd;
    std::sort(new_program.cmds.begin(), new_program.cmds.end());
    return new_program;
  }

  constexpr operator ProgramView() const noexcept {
    return ProgramView(this->metadata, this->args.data(), this->args.size(), this->cmds.data(), this->cmds.size());
  }

  ArgMap operator()(std::span<char const *> args) const {
    try {
      return parse(*this, args);
    } catch (UserError const &err) {
      std::exit(this->metadata.error_handler(*this, err));
    }
  }

  ArgMap operator()(int argc, char const *argv[]) const {
    return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
  }

private:
};

// +-----------+
// | utilities |
// +-----------+

void print_full_help(ProgramView const, std::ostream & = std::cout) noexcept;

// +------------+
// | formatting |
// +------------+

class HelpFormatter {
public:
  HelpFormatter(ProgramView const, std::ostream &);

  void print_title() const noexcept;
  void print_intro() const noexcept;
  void print_long_usage() const noexcept;
  void print_help() const noexcept;
  void print_details() const noexcept;

  std::size_t help_padding_size() const noexcept;

private:
  std::ostream &out;
  ProgramView const program;

  void print_arg_help(auto const &arg, std::size_t const padding_size) const noexcept {
    using std::views::drop;
    auto const description = arg.format_for_help_description();
    // -8 because we print 4 spaces of left margin and 4 spaces of indentation for descriptions longer than 1 line
    // then -4 again because we add 4 spaces between the arg usage and description
    auto const description_lines = limit_within(description, program.metadata.msg_width - padding_size - 8 - 4);

    out << fmt::format("    {:<{}}    {}\n", arg.format_for_help_index(), padding_size,
                       fmt::join(description_lines.front(), " "));

    for (auto const &line : description_lines | drop(1)) {
      // the same 4 spaces of left margin, then additional 4 spaces of indentation
      out << fmt::format("    {: >{}}        {}\n", ' ', padding_size, fmt::join(line, " "));
    }
  }
};

// +------------------------------------+
// | implementation of action functions |
// +------------------------------------+

namespace act::fn {

// +--------+
// | assign |
// +--------+

template <concepts::ExternalType T>
void assign_to(ArgMap &map, std::string_view const name, T value) {
  auto [it, inserted] = map.args.try_emplace(name, value);
  if (!inserted)
    throw DuplicateAssignment(name);
}

template <concepts::BuiltinType T>
void assign(ProgramView const, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  if (arg.type != ArgType::FLG && parsed_value)
    assign_to(map, arg.name, convert<T>(*parsed_value));
  else
    assign_to(map, arg.name, std::get<T>(arg.implicit_value));
}

// +--------+
// | append |
// +--------+

template <concepts::BuiltinType Elem>
void append_to(ArgMap &map, std::string_view const name, Elem value) {
  if (auto list = map.args.find(name); list != map.args.end()) {
    std::get<std::vector<Elem>>(list->second.value).emplace_back(std::move(value));
  } else {
    assign_to(map, name, std::vector<Elem>{std::move(value)});
  }
}

template <concepts::BuiltinType Elem>
void append(ProgramView const, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  if (arg.type != ArgType::FLG && parsed_value)
    append_to<Elem>(map, arg.name, convert<Elem>(*parsed_value));
  else
    append_to<Elem>(map, arg.name, std::get<Elem>(arg.implicit_value));
}

// +-----+
// | csv |
// +-----+

template <concepts::BuiltinType Elem>
void csv(ProgramView const, ArgMap &map, Arg const &arg, std::optional<std::string_view> const parsed_value) {
  assign_to(map, arg.name, convert<std::vector<Elem>>(*parsed_value));
}

} // namespace act::fn

} // namespace opzioni

// +-----------------------------------------------+
// | forward declarations of our `fmt::formatter`s |
// +-----------------------------------------------+

template <>
struct fmt::formatter<std::monostate>;

template <>
struct fmt::formatter<std::vector<bool>>;

#endif // OPZIONI_H
