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
struct ProgramView;

template <std::size_t ArgsSize = 0, std::size_t CmdsSize = 0>
class Program;

consteval void validate_arg(Arg const &) noexcept;

template <std::size_t N>
consteval void validate_args(std::array<Arg, N> const &, Arg const &) noexcept;

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
  BuiltinVariant set_value{};
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
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
    arg.is_required = false;
    arg.action_fn = actions::append<Elem>;
    if (!this->is_required)
      arg.default_setter = set_empty_vector<Elem>;
    return arg;
  }

  template <concepts::BuiltinType Elem>
  consteval Arg csv_of() const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use the csv action because they do not take values from the command-line";
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
    arg.is_required = false;
    arg.action_fn = actions::csv<Elem>;
    if (!this->is_required)
      arg.default_setter = set_empty_vector<Elem>;
    return arg;
  }

  template <concepts::BuiltinType Elem = std::string_view>
  consteval Arg gather(std::size_t amount) const noexcept {
    if (this->type == ArgType::FLG)
      throw "Flags cannot use gather because they do not take values from the command-line";
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
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
    auto arg = Arg::With(*this, value, this->set_value);
    arg.action_fn = actions::assign<T>;
    arg.is_required = false;
    return arg;
  }

  consteval Arg otherwise(char const *value) const noexcept { return otherwise(std::string_view(value)); }

  consteval Arg otherwise(DefaultValueSetter setter) const noexcept {
    auto arg = Arg::With(*this, std::monostate{}, this->set_value);
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

  template <concepts::BuiltinType T>
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
  static consteval Arg With(Arg const &other, BuiltinVariant default_value, BuiltinVariant set_value) noexcept {
    return Arg{.type = other.type,
               .name = other.name,
               .abbrev = other.abbrev,
               .description = other.description,
               .is_required = other.is_required,
               .default_value = default_value,
               .set_value = set_value,
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
  return Arg{.type = ArgType::OPT, .name = name, .abbrev = abbrev, .default_value = ""};
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

struct ParsedOption {
  std::string_view name;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

constexpr ParsedOption parse_option(std::string_view const) noexcept;

// +---------+
// | Program |
// +---------+

ArgMap parse(ProgramView const program, std::span<char const *> args);

struct ProgramMetadata {
  std::string_view name{};
  std::string_view version{};
  std::string_view title{};
  std::string_view introduction{};
  std::string_view description{};

  std::size_t msg_width = 100;
  std::size_t positionals_amount = 0;
  ErrorHandler error_handler = print_error;

  constexpr auto operator<=>(ProgramMetadata const &) const noexcept = default;
};

struct ProgramView {
  ProgramMetadata metadata;
  std::span<Arg const> args;
  std::span<ProgramView const> cmds;

  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;

  constexpr auto operator<=>(ProgramView const &other) const noexcept {
    return this->metadata.name <=> other.metadata.name;
  }
};

// struct Cmd {
//   ProgramView program;

//   consteval Cmd() = default;
//   consteval Cmd(Cmd const &other) : program(other.program) {}
//   consteval Cmd(ProgramView program) : program(program) {}
// };

consteval auto operator*(ProgramView const lhs, ProgramView const rhs) noexcept {
  if (lhs.metadata.name == rhs.metadata.name)
    throw "Trying to add command with a duplicate name";
  return std::array{lhs, rhs};
}

template <std::size_t N>
consteval auto operator*(std::array<ProgramView, N> const cmds, ProgramView const other) noexcept {
  // validate_arg(other);
  // validate_args(cmds, other);

  std::array<ProgramView, N + 1> newcmds;
  std::copy_n(cmds.begin(), N, newcmds.begin());
  newcmds[N] = other;
  return newcmds;
}

template <std::size_t ArgsSize, std::size_t CmdsSize>
class Program {
public:
  ProgramMetadata metadata{};
  std::array<Arg, ArgsSize> args;
  std::array<ProgramView, CmdsSize> cmds;

  consteval Program() = default;
  consteval Program(std::string_view name) : Program(name, {}) {}
  consteval Program(std::string_view name, std::string_view title) : metadata(name, title) {}

  consteval Program(ProgramView const &other) : metadata(other.metadata) {
    if constexpr (ArgsSize > 0)
      if (other.args.size() > 0)
        std::copy_n(other.args.begin(), ArgsSize, args.begin());
    if constexpr (CmdsSize > 0)
      if (other.args.size() > 0)
        std::copy_n(other.cmds.begin(), CmdsSize, cmds.begin());
  }

  consteval auto intro(std::string_view introduction) const noexcept {
    auto program = *this;
    program.metadata.introduction = introduction;
    return program;
  }

  consteval auto details(std::string_view description) const noexcept {
    auto program = *this;
    program.metadata.description = description;
    return program;
  }

  consteval auto version(std::string_view version) const noexcept {
    auto program = *this;
    program.metadata.version = version;
    return program;
  }

  consteval auto max_width(std::size_t msg_width) const noexcept {
    auto program = *this;
    program.metadata.msg_width = msg_width;
    return program;
  }

  consteval auto on_error(ErrorHandler error_handler) const noexcept {
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
    for (std::size_t i = 0; i < N; ++i)
      newprogram.cmds[i] = cmds[i];
    std::sort(newprogram.cmds.begin(), newprogram.cmds.end());
    return newprogram;
  }

  constexpr operator ProgramView() const noexcept { return ProgramView(this->metadata, this->args, this->cmds); }

  ArgMap operator()(std::span<char const *> args) const noexcept {
    try {
      return parse(*this, args);
    } catch (UserError const &err) {
      if (this->metadata.error_handler == nullptr)
        std::exit(-1);
      std::exit(this->metadata.error_handler(*this, err));
    }
  }

  ArgMap operator()(int argc, char const *argv[]) const noexcept {
    return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
  }
};

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

// +-----------------+
// | parsing helpers |
// +-----------------+

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr ProgramView const *is_command(ProgramView const, std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view is_short_flags(ProgramView const, std::string_view const) noexcept;
constexpr std::string_view is_long_flag(ProgramView const, std::string_view const) noexcept;
constexpr std::optional<ParsedOption> is_option(ProgramView const, std::string_view const) noexcept;
constexpr bool is_flag(ProgramView const, std::string_view const) noexcept;

std::size_t assign_command(ArgMap &, std::span<char const *>, ProgramView const);
std::size_t assign_positional(ProgramView const, ArgMap &, std::span<char const *>, std::size_t const);
std::size_t assign_many_flags(ProgramView const, ArgMap &, std::string_view);
std::size_t assign_flag(ProgramView const, ArgMap &, std::string_view);
std::size_t assign_option(ProgramView const, ArgMap &, std::span<char const *>, ParsedOption const);

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
  void print_description() const noexcept;

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
    assign_to(map, arg.name, std::get<T>(arg.set_value));
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
    append_to<Elem>(map, arg.name, std::get<Elem>(arg.set_value));
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
