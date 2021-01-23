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

// +---------+
// | actions |
// +---------+
namespace actions {

using Signature = void (*)(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType T>
void assign(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType Elem>
void append(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

template <concepts::BuiltinType Elem>
void csv(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_help(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

void print_version(ProgramView const, ArgMap &, Arg const &, std::optional<std::string_view> const);

} // namespace actions

// +-----------+
// | arguments |
// +-----------+

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

using DefaultValueSetter = void (*)(ArgValue &);

template <concepts::BuiltinType T>
void set_empty_vector(ArgValue &arg) noexcept {
  arg.value = std::vector<T>{};
}

// +-----+
// | Arg |
// +-----+

enum struct ArgType { POS, OPT, FLG };

struct Arg {
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view description{};
  bool is_required = false;
  BuiltinVariant default_value{};
  BuiltinVariant implicit_value{};
  actions::Signature action_fn = actions::assign<std::string_view>;
  std::size_t gather_amount = 1;
  DefaultValueSetter default_setter = nullptr;

  consteval Arg action(actions::Signature action_fn) const noexcept {
    auto arg = *this;
    arg.action_fn = action_fn;
    return arg;
  }

  template <concepts::BuiltinType Elem = std::string_view>
  consteval Arg append() const noexcept {
    auto arg = Arg::With(*this, std::monostate{}, this->implicit_value);
    arg.action_fn = actions::append<Elem>;
    if (!this->is_required)
      arg.default_setter = set_empty_vector<Elem>;
    return arg;
  }

  template <concepts::BuiltinType Elem = std::string_view>
  consteval Arg csv() const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use the csv action because they do not take values from the command-line";
    auto arg = Arg::With(*this, std::monostate{}, this->implicit_value);
    arg.action_fn = actions::csv<Elem>;
    if (!this->is_required)
      arg.default_setter = set_empty_vector<Elem>;
    return arg;
  }

  template <concepts::BuiltinType Elem = std::string_view>
  consteval Arg gather(std::size_t amount) const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use gather because they do not take values from the command-line";
    auto arg = Arg::With(*this, std::monostate{}, this->implicit_value);
    arg.gather_amount = amount;
    arg.action_fn = actions::append<Elem>;
    if (!this->is_required)
      arg.default_setter = set_empty_vector<Elem>;
    return arg;
  }

  template <concepts::BuiltinType Elem = std::string_view>
  consteval Arg gather() const noexcept {
    return gather<Elem>(0);
  }

  consteval Arg help(std::string_view description) const noexcept {
    auto arg = *this;
    arg.description = description;
    return arg;
  }

  template <concepts::BuiltinType T>
  consteval Arg of() const noexcept {
    auto arg = *this;
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  template <concepts::BuiltinType T>
  consteval Arg otherwise(T value) const noexcept {
    auto arg = Arg::With(*this, value, this->implicit_value);
    arg.action_fn = actions::assign<T>;
    arg.default_setter = nullptr;
    arg.is_required = false;
    return arg;
  }

  consteval Arg otherwise(char const *value) const noexcept { return otherwise(std::string_view(value)); }

  consteval Arg otherwise(DefaultValueSetter setter) const noexcept {
    auto arg = Arg::With(*this, std::monostate{}, this->implicit_value);
    arg.default_setter = setter;
    arg.is_required = false;
    return arg;
  }

  consteval Arg required() const noexcept {
    auto arg = Arg::With(*this, std::monostate{}, this->implicit_value);
    arg.default_setter = nullptr;
    arg.is_required = true;
    return arg;
  }

  template <concepts::BuiltinType T>
  consteval Arg implicitly(T value) const noexcept {
    if (this->type == ArgType::POS)
      throw "Positionals cannot use implicit value because they always take a value from the command-line";
    auto arg = Arg::With(*this, this->default_value, value);
    arg.action_fn = actions::assign<T>;
    return arg;
  }

  consteval Arg implicitly(char const *value) const noexcept { return implicitly(std::string_view(value)); }

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
  return std::array{lhs, rhs};
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
                       .default_value = false,
                       .implicit_value = true,
                       .action_fn = actions::assign<bool>};
  validate_arg(arg);
  return arg;
}

consteval Arg Flg(std::string_view name) noexcept { return Flg(name, {}); }

consteval Arg Opt(std::string_view name, std::string_view abbrev) noexcept {
  auto const arg = Arg{.type = ArgType::OPT, .name = name, .abbrev = abbrev, .default_value = ""};
  validate_arg(arg);
  return arg;
}

consteval Arg Opt(std::string_view name) noexcept { return Opt(name, {}); }

consteval Arg Pos(std::string_view name) noexcept {
  auto const arg = Arg{.type = ArgType::POS, .name = name, .is_required = true};
  validate_arg(arg);
  return arg;
}

consteval Arg Help(std::string_view description) noexcept {
  return Flg("help", "h").help(description).action(actions::print_help);
}

consteval Arg Help() noexcept { return Help("Display this information"); }

consteval Arg Version(std::string_view description) noexcept {
  return Flg("version", "V").help(description).action(actions::print_version);
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

consteval auto operator*(ProgramView const lhs, ProgramView const rhs) noexcept {
  if (lhs == rhs)
    throw "Trying to add command with a duplicate name";
  return std::array{lhs, rhs};
}

template <std::size_t N>
consteval auto operator*(std::array<ProgramView, N> const cmds, ProgramView const other) noexcept {
  validate_cmds(cmds, other);

  std::array<ProgramView, N + 1> newcmds;
  std::copy_n(cmds.begin(), N, newcmds.begin());
  newcmds[N] = other;
  return newcmds;
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

  template <std::size_t N>
  consteval Program<N, CmdsSize> operator+(std::array<Arg, N> args) const noexcept {
    Program<N, CmdsSize> newprogram(*this);
    std::array<Arg, N> positionals, others;

    auto const partition_result =
        std::ranges::partition_copy(args, positionals.begin(), others.begin(), &Arg::is_positional);
    newprogram.metadata.positionals_amount = std::ranges::distance(positionals.begin(), partition_result.out1);

    auto const copy_result =
        std::ranges::copy_n(positionals.begin(), newprogram.metadata.positionals_amount, newprogram.args.begin());
    std::ranges::copy_n(others.begin(), N - newprogram.metadata.positionals_amount, copy_result.out);

    std::sort(copy_result.out, newprogram.args.end());
    return newprogram;
  }

  template <std::size_t N>
  consteval Program<ArgsSize, N> operator+(std::array<ProgramView, N> cmds) const noexcept {
    Program<ArgsSize, N> newprogram(*this);
    std::ranges::copy(cmds, newprogram.cmds.begin());
    std::sort(newprogram.cmds.begin(), newprogram.cmds.end()); // don't know why can't use std::ranges::sort
    return newprogram;
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

// +---------------------------+
// | implementation of actions |
// +---------------------------+

namespace actions {

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

} // namespace actions

} // namespace opzioni

// +-----------------------------------------------+
// | forward declarations of our `fmt::formatter`s |
// +-----------------------------------------------+

template <>
struct fmt::formatter<std::monostate>;

template <>
struct fmt::formatter<std::vector<bool>>;

#endif // OPZIONI_H
