#include "opzioni.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <optional>
#include <tuple>
#include <variant>

namespace opzioni {

std::string builtin2str(BuiltinVariant const &variant) noexcept {
  auto const _2str = [](auto const &val) { return fmt::to_string(val); };
  return std::visit(_2str, variant);
}

int print_error(ProgramView const program, UserError const &err) noexcept {
  std::cerr << limit_string_within(err.what(), program.metadata.msg_width) << nl;
  return -1;
}

int print_error_and_usage(ProgramView const program, UserError const &err) noexcept {
  print_error(program, err);
  HelpFormatter formatter(program, std::cerr);
  std::cerr << nl;
  formatter.print_long_usage();
  return -1;
}

int rethrow(ProgramView const, UserError const &ue) { throw ue; }

// +-----+
// | Arg |
// +-----+

std::string Arg::format_base_usage() const noexcept {
  if (type == ArgType::POS)
    return std::string(name);

  auto const dashes = name.length() > 1 ? "--" : "-";
  if (type == ArgType::OPT) {
    auto val = fmt::format("<{}>", has_abbrev() ? abbrev : name);
    if (has_set())
      val = "[" + val + "]";
    return fmt::format("{}{} {}", dashes, name, val);
  }

  return fmt::format("{}{}", dashes, name);
}

std::string Arg::format_for_help_description() const noexcept {
  auto const format = [this](auto const &default_value, auto const &set_value) {
    return fmt::format(description, fmt::arg("name", this->name), fmt::arg("abbrev", this->abbrev),
                       fmt::arg("default_value", default_value), fmt::arg("set_value", set_value),
                       fmt::arg("gather_amount", this->gather_amount));
  };
  ArgValue default_value{};
  set_default_to(default_value); // gotta use this because of DefaultValueSetter
  return std::visit(format, default_value.value, this->set_value);
}

std::string Arg::format_for_help_index() const noexcept {
  auto const base_usage = format_base_usage();

  if (type == ArgType::POS)
    return base_usage;

  if (has_abbrev())
    return fmt::format("-{}, {}", abbrev, base_usage);

  if (name.length() == 1)
    return base_usage;

  return fmt::format("    {}", base_usage);
}

std::string Arg::format_for_usage_summary() const noexcept {
  auto format = format_base_usage();

  if (type == ArgType::POS)
    format = "<" + format + ">";

  if (!is_required)
    format = "[" + format + "]";

  return format;
}

std::string Cmd::format_for_help_description() const noexcept { return std::string(program->metadata.introduction); }

std::string Cmd::format_for_help_index() const noexcept { return format_for_usage_summary(); }

std::string Cmd::format_for_usage_summary() const noexcept { return std::string(program->metadata.name); }

auto Cmd::operator<=>(Cmd const &other) const noexcept {
  return this->program->metadata.name <=> other.program->metadata.name;
}

// +---------+
// | Program |
// +---------+

Program &Program::intro(std::string_view introduction) noexcept {
  this->metadata.introduction = introduction;
  return *this;
}

Program &Program::details(std::string_view description) noexcept {
  this->metadata.description = description;
  return *this;
}

Program &Program::v(std::string_view version) noexcept {
  this->metadata.version = version;
  return *this;
}

Program &Program::max_width(std::size_t msg_width) noexcept {
  this->metadata.msg_width = msg_width;
  return *this;
}

Program &Program::on_error(ErrorHandler error_handler) noexcept {
  this->error_handler = error_handler;
  return *this;
}

Program &Program::add(Arg arg) {
  _args.push_back(arg);
  metadata.positionals_amount += (arg.type == ArgType::POS);
  return *this;
}

Program &Program::add(Cmd cmd) {
  if (has_cmd(*this, cmd.program->metadata.name))
    throw ArgumentAlreadyExists(cmd.program->metadata.name);
  _cmds.push_back(cmd);
  return *this;
}

ArgMap Program::operator()(int argc, char const *argv[]) const {
  return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
}

ArgMap Program::operator()(std::span<char const *> args) const {
  try {
    auto map = parse(args);
    check_contains_required(map);
    set_defaults(map);
    return map;
  } catch (UserError const &err) {
    if (this->error_handler == nullptr)
      std::exit(-1);
    std::exit(this->error_handler(*this, err));
  }
}

ArgMap Program::parse(std::span<char const *> args) const {
  ArgMap map;
  if (args.size() > 0) {
    map.exec_path = std::string(args[0]);
    std::size_t current_positional_idx = 0;
    for (std::size_t index = 1; index < args.size();) {
      auto const arg = std::string_view(args[index]);
      if (is_dash_dash(arg)) {
        // +1 to ignore the dash-dash
        for (std::size_t offset = index + 1; offset < args.size();) {
          offset += assign_positional(*this, map, args.subspan(offset), current_positional_idx);
          ++current_positional_idx;
        }
        index = args.size();
      } else if (auto cmd = is_command(*this, arg); cmd != nullptr) {
        index += assign_command(map, args.subspan(index), *cmd);
      } else if (looks_positional(arg)) {
        index += assign_positional(*this, map, args.subspan(index), current_positional_idx);
        ++current_positional_idx;
      } else if (auto const flags = is_short_flags(*this, arg); !flags.empty()) {
        index += assign_many_flags(map, flags);
      } else if (auto const flag = is_long_flag(*this, arg); !flag.empty()) {
        index += assign_flag(map, flag);
      } else if (auto const option = is_option(*this, arg); option.has_value()) {
        index += assign_option(map, args.subspan(index), *option);
      } else {
        throw UnknownArgument(arg);
      }
    }
  }
  return map;
}

void Program::check_contains_required(ArgMap const &map) const {
  using std::ranges::transform;
  using std::views::filter;
  auto get_name = [](auto const &arg) -> std::string_view { return arg.name; };
  auto wasnt_parsed = [&map](auto const &arg) { return !map.has(arg.name); };
  auto is_required = [](auto const &arg) { return arg.is_required; };
  std::vector<std::string_view> missing_arg_names;
  auto insert = std::back_inserter(missing_arg_names);
  transform(_args | filter(wasnt_parsed) | filter(is_required), insert, get_name);
  if (!missing_arg_names.empty())
    throw MissingRequiredArguments(missing_arg_names);
}

void Program::set_defaults(ArgMap &map) const noexcept {
  using std::views::filter;
  auto wasnt_parsed = [&map](auto const &arg) { return !map.has(arg.name); };
  for (auto const &arg : _args | filter(wasnt_parsed) | filter(&Arg::has_default))
    arg.set_default_to(map.args[arg.name]);
}

// +--------------------+
// | arg and cmd search |
// +--------------------+

constexpr auto find_arg(ProgramView const program, std::string_view name, ArgType type) noexcept {
  return std::ranges::find_if(program.args, [name, type](auto const &arg) {
    return arg.type == type && (arg.name == name || (arg.has_abbrev() && arg.abbrev == name));
  });
}

constexpr bool has_arg(ProgramView const program, std::string_view name, ArgType type) noexcept {
  return find_arg(program, name, type) != program.args.end();
}
constexpr bool has_flg(ProgramView const program, std::string_view name) noexcept {
  return has_arg(program, name, ArgType::FLG);
}
constexpr bool has_opt(ProgramView const program, std::string_view name) noexcept {
  return has_arg(program, name, ArgType::OPT);
}
constexpr bool has_pos(ProgramView const program, std::string_view name) noexcept {
  return has_arg(program, name, ArgType::POS);
}

constexpr auto find_cmd(ProgramView const program, std::string_view name) noexcept {
  return std::ranges::find(program.cmds, name, [](auto const &cmd) { return cmd.program->metadata.name; });
}

constexpr bool has_cmd(ProgramView const program, std::string_view name) noexcept {
  return find_cmd(program, name) != program.cmds.end();
}

// +-----------------+
// | parsing helpers |
// +-----------------+

constexpr bool is_dash_dash(std::string_view const whole_arg) noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

constexpr Cmd const *is_command(ProgramView const program, std::string_view const whole_arg) noexcept {
  if (auto const cmd = find_cmd(program, whole_arg); cmd != program.cmds.end())
    return &*cmd;
  return nullptr;
}

constexpr bool looks_positional(std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  return num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string_view::npos);
}

constexpr std::string_view is_short_flags(ProgramView const program, std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  auto const all_short_flags = std::all_of(
      flags.begin(), flags.end(), [program](char const c) { return is_flag(program, std::string_view(&c, 1)); });
  if (num_of_dashes == 1 && flags.length() >= 1 && all_short_flags)
    return flags;
  return {};
}

constexpr std::string_view is_long_flag(ProgramView const program, std::string_view const whole_arg) noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2 && is_flag(program, name))
    return name;
  return {};
}

constexpr std::optional<ParsedOption> is_option(ProgramView const program, std::string_view const whole_arg) noexcept {
  auto const parsed_option = parse_option(whole_arg);
  if (has_opt(program, parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

constexpr bool is_flag(ProgramView const program, std::string_view const name) noexcept {
  return has_flg(program, name);
}

// +---------------------+
// | parsing assignments |
// +---------------------+

std::size_t assign_command(ArgMap &map, std::span<char const *> args, Cmd const &cmd) {
  auto const exec_path = fmt::format("{} {}", map.exec_path, cmd.program->metadata.name);
  args[0] = exec_path.data();
  map.cmd_name = cmd.program->metadata.name;
  map.cmd_args = std::make_shared<ArgMap>(std::move((*cmd.program)(args)));
  return args.size();
}

std::size_t assign_positional(ProgramView const program, ArgMap &map, std::span<char const *> args,
                              std::size_t const positional_idx) {
  if (positional_idx >= program.metadata.positionals_amount) {
    throw UnexpectedPositional(args[0], program.metadata.positionals_amount);
  }
  auto const arg = program.args[positional_idx];
  // if gather amount is 0, we gather everything else
  auto const gather_amount = arg.gather_amount == 0 ? args.size() : arg.gather_amount;
  if (gather_amount > args.size()) {
    throw MissingValue(arg.name, gather_amount, args.size());
  }
  for (std::size_t count = 0; count < gather_amount; ++count) {
    arg.action_fn(program, map, arg, args[count]);
  }
  return gather_amount;
}

std::size_t Program::assign_many_flags(ArgMap &map, std::string_view flags) const {
  for (auto const flag : flags) {
    assign_flag(map, std::string_view(&flag, 1));
  }
  return 1;
}

std::size_t Program::assign_flag(ArgMap &map, std::string_view flag) const {
  auto const &arg = *find_arg(*this, flag, ArgType::FLG);
  arg.action_fn(*this, map, arg, std::nullopt);
  return 1;
}

std::size_t Program::assign_option(ArgMap &map, std::span<char const *> args, ParsedOption const option) const {
  auto const &arg = *find_arg(*this, option.name, ArgType::OPT);
  auto const gather_amount = arg.gather_amount == 0 ? args.size() - 1 : arg.gather_amount;
  if (option.value) {
    if (gather_amount != 1) {
      throw MissingValue(arg.name, gather_amount, 1);
    }
    arg.action_fn(*this, map, arg, *option.value);
    return 1;
  } else if (gather_amount < args.size() && looks_positional(args[1])) {
    // + 1 everywhere because 0 is the index in `args` that the option is,
    // so 1 is where we start to parse values up to gather amount
    std::size_t count = 0;
    do {
      auto const value = std::string_view(args[count + 1]);
      arg.action_fn(*this, map, arg, value);
      ++count;
    } while (count < gather_amount);
    return gather_amount + 1;
  } else if (arg.has_set()) {
    arg.action_fn(*this, map, arg, std::nullopt);
    return 1;
  } else {
    throw MissingValue(arg.name, gather_amount, args.size() - 1);
  }
}

constexpr ParsedOption parse_option(std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const eq_idx = whole_arg.find('=', num_of_dashes);
  bool const has_equals = eq_idx != std::string_view::npos;
  if (num_of_dashes == 1) {
    // short option, e.g. `-O`
    auto const name = whole_arg.substr(1, 1);
    if (has_equals) {
      if (whole_arg.length() > 3) {
        // has equals followed by some value, e.g. `-O=2`
        return {name, whole_arg.substr(3)};
      }
      // should this `-O=` be handled like this?
      return {name, ""};
    }
    if (whole_arg.length() > 2) {
      // only followed by some value, e.g. `-O2`
      return {name, whole_arg.substr(2)};
    }
    // case left: has no value (next CLI argument could be it)
    return {name, std::nullopt};
  }
  if (num_of_dashes == 2 && whole_arg.length() > 3) {
    // long option, e.g. `--name`
    if (has_equals) {
      auto const name = whole_arg.substr(2, eq_idx - 2);
      auto const value = whole_arg.substr(eq_idx + 1);
      return {name, value};
    }
    // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
    return {whole_arg.substr(2), std::nullopt};
  }
  // not an option
  return {"", std::nullopt};
}

// +------------+
// | formatting |
// +------------+

HelpFormatter::HelpFormatter(ProgramView const program, std::ostream &out) : out(out), program(program) {}

std::size_t HelpFormatter::help_padding_size() const noexcept {
  using std::views::transform;
  auto const required_length = [](auto const &arg) -> std::size_t { return arg.format_for_help_index().length(); };
  std::size_t const required_length_args =
      program.args.empty() ? 0 : std::ranges::max(program.args | transform(required_length));
  std::size_t const required_length_cmds =
      program.cmds.empty() ? 0 : std::ranges::max(program.cmds | transform(required_length));
  return std::max(required_length_args, required_length_cmds);
}

void HelpFormatter::print_title() const noexcept {
  out << program.metadata.name;
  if (!program.metadata.version.empty())
    out << ' ' << program.metadata.version;
  if (!program.metadata.title.empty())
    out << " - " << program.metadata.title;
  out << nl;
}

void HelpFormatter::print_intro() const noexcept {
  if (program.metadata.introduction.length() <= program.metadata.msg_width) {
    out << program.metadata.introduction << nl;
  } else {
    out << limit_string_within(program.metadata.introduction, program.metadata.msg_width) << nl;
  }
}

void HelpFormatter::print_long_usage() const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::transform;
  using std::views::drop, std::views::take;

  std::vector<std::string> words;
  words.reserve(1 + program.cmds.size() + program.args.size());

  auto insert = std::back_inserter(words);
  words.push_back(std::string(program.metadata.name));
  transform(program.args, insert, &Arg::format_for_usage_summary);

  if (program.cmds.size() == 1) {
    words.push_back(format("{{{}}}", program.cmds.front().format_for_usage_summary()));
  } else if (program.cmds.size() > 1) {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", program.cmds.front().format_for_usage_summary()));
    transform(program.cmds | drop(1) | take(program.cmds.size() - 2), insert, &Cmd::format_for_usage_summary);
    words.push_back(format("{}}}", program.cmds.back().format_for_usage_summary()));
  }

  // -4 because we'll later print a left margin of 4 spaces
  auto const split_lines = limit_within(words, program.metadata.msg_width - 4);
  out << "Usage:\n";
  for (auto const &line : split_lines) {
    out << format("    {}\n", join(line, " "));
  }
}

void HelpFormatter::print_help() const noexcept {
  std::string_view pending_nl = "";
  // using same padding size for all arguments so they stay aligned
  auto const padding_size = help_padding_size();

  if (program.metadata.positionals_amount > 0) {
    out << "Positionals:\n";
    for (auto const &arg : program.args | std::views::take(program.metadata.positionals_amount)) {
      print_arg_help(arg, padding_size);
    }
    pending_nl = "\n";
  }

  if (program.args.size() > program.metadata.positionals_amount) {
    out << pending_nl << "Options & Flags:\n";
    for (auto const &arg : program.args | std::views::drop(program.metadata.positionals_amount)) {
      print_arg_help(arg, padding_size);
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!program.cmds.empty()) {
    out << pending_nl << "Commands:\n";
    for (auto const &arg : program.cmds) {
      print_arg_help(arg, padding_size);
    }
  }
}

void HelpFormatter::print_description() const noexcept {
  if (program.metadata.description.empty())
    return;
  if (program.metadata.description.length() <= program.metadata.msg_width)
    out << program.metadata.description << nl;
  else
    out << limit_string_within(program.metadata.description, program.metadata.msg_width) << nl;
}

// +-----------+
// | utilities |
// +-----------+

void print_full_help(ProgramView const program, std::ostream &ostream) noexcept {
  HelpFormatter formatter(program, ostream);
  formatter.print_title();
  if (!program.metadata.introduction.empty()) {
    ostream << nl;
    formatter.print_intro();
  }
  ostream << nl;
  formatter.print_long_usage();
  ostream << nl;
  formatter.print_help();
  ostream << nl;
  formatter.print_description();
}

// +---------+
// | actions |
// +---------+

namespace actions {

void print_help(ProgramView const program, ArgMap &, Arg const &, std::optional<std::string_view> const) {
  print_full_help(program);
  std::exit(0);
}

void print_version(ProgramView const program, ArgMap &, Arg const &, std::optional<std::string_view> const) {
  fmt::print("{} {}\n", program.metadata.name, program.metadata.version);
  std::exit(0);
}

} // namespace actions

} // namespace opzioni

// +-----------------------------------+
// | specializations of fmt::formatter |
// +-----------------------------------+

template <>
struct fmt::formatter<std::monostate> : fmt::formatter<std::string_view> {
  using fmt::formatter<std::string_view>::parse;

  template <typename FormatContext>
  auto format(std::monostate const &_, FormatContext &ctx) {
    return fmt::formatter<std::string_view>::format("", ctx);
  }
};

template <>
struct fmt::formatter<std::vector<bool>> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.end(); }

  template <typename FormatContext>
  auto format(std::vector<bool> const &vec, FormatContext &ctx) {
    auto out = fmt::format_to(ctx.out(), "{{");
    for (auto &&elem : vec)
      out = fmt::format_to(out, "{}", elem);
    return fmt::format_to(out, "}}");
  }
};
