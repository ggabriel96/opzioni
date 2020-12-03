#include "opzioni.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <optional>
#include <tuple>
#include <variant>

namespace opzioni {

std::string builtin2str(BuiltinType const &variant) noexcept {
  auto const _2str = [](auto const &val) { return fmt::format("{}", val); };
  return std::visit(_2str, variant);
}

int print_error(Program const &program, UserError const &err) noexcept {
  auto const see_help = fmt::format("See `{} --help` for more information", program.path);
  std::cerr << limit_string_within(err.what(), program.msg_width) << nl;
  std::cerr << nl << limit_string_within(see_help, program.msg_width) << nl;
  return -1;
}

int print_error_and_usage(Program const &program, UserError const &err) noexcept {
  print_error(program, err);
  HelpFormatter formatter(program, std::cerr);
  std::cerr << nl;
  formatter.print_long_usage();
  return -1;
}

// +-----+
// | Arg |
// +-----+

template <>
std::string Arg<ArgumentType::POSITIONAL>::format_base_usage() const noexcept {
  return std::string(name);
}

template <>
std::string Arg<ArgumentType::POSITIONAL>::format_usage() const noexcept {
  if (is_required)
    return "<" + format_base_usage() + ">";
  return "[<" + format_base_usage() + ">]";
}

template <>
std::string Arg<ArgumentType::POSITIONAL>::format_help_usage() const noexcept {
  return format_base_usage();
}

template <>
std::string Arg<ArgumentType::OPTION>::format_base_usage() const noexcept {
  auto const dashes = name.length() > 1 ? "--" : "-";
  auto val = fmt::format("<{}>", has_abbrev() ? abbrev : name);
  if (set_value)
    val = "[" + val + "]";
  return fmt::format("{}{} {}", dashes, name, val);
}

template <>
std::string Arg<ArgumentType::OPTION>::format_usage() const noexcept {
  if (is_required)
    return format_base_usage();
  return "[" + format_base_usage() + "]";
}

template <>
std::string Arg<ArgumentType::OPTION>::format_help_usage() const noexcept {
  if (has_abbrev())
    return fmt::format("-{}, {}", abbrev, format_base_usage());
  return format_base_usage();
}

template <>
std::string Arg<ArgumentType::FLAG>::format_base_usage() const noexcept {
  auto const dashes = name.length() > 1 ? "--" : "-";
  return fmt::format("{}{}", dashes, name);
}

template <>
std::string Arg<ArgumentType::FLAG>::format_usage() const noexcept {
  if (is_required)
    return format_base_usage();
  return "[" + format_base_usage() + "]";
}

template <>
std::string Arg<ArgumentType::FLAG>::format_help_usage() const noexcept {
  if (has_abbrev())
    return fmt::format("-{}, {}", abbrev, format_base_usage());
  return format_base_usage();
}

template <>
std::string Arg<ArgumentType::POSITIONAL>::format_help_description() const noexcept {
  if (default_value) {
    return fmt::format("{} (default: {})", description, std::visit(builtin2str, *default_value));
  }
  return std::string(description);
}

template <>
std::string Arg<ArgumentType::OPTION>::format_help_description() const noexcept {
  std::string format(description);
  if (set_value || default_value) {
    format += " (";
    if (set_value)
      format += fmt::format("valueless: {}", std::visit(builtin2str, *set_value));
    if (default_value) {
      if (set_value)
        format += ", ";
      format += fmt::format("default: {}", std::visit(builtin2str, *default_value));
    }
    format += ")";
  }
  return format;
}

template <>
std::string Arg<ArgumentType::FLAG>::format_help_description() const noexcept {
  std::string format(description);
  if (set_value || default_value) {
    format += " (";
    if (set_value)
      format += fmt::format("sets: {}", std::visit(builtin2str, *set_value));
    if (default_value) {
      if (set_value)
        format += ", ";
      format += fmt::format("default: {}", std::visit(builtin2str, *default_value));
    }
    format += ")";
  }
  return format;
}

std::string Command::format_help_usage() const noexcept { return std::string(name); }

std::string Command::format_help_description() const noexcept { return std::string(spec->introduction); }

// +---------+
// | Program |
// +---------+

Program &Program::intro(std::string_view introduction) noexcept {
  this->introduction = introduction;
  return *this;
}

Program &Program::details(std::string_view description) noexcept {
  this->description = description;
  return *this;
}

Program &Program::max_width(std::size_t msg_width) noexcept {
  this->msg_width = msg_width;
  return *this;
}

Program &Program::on_error(opzioni::error_handler error_handler) noexcept {
  this->error_handler = error_handler;
  return *this;
}

Program &Program::auto_help() noexcept {
  return this->auto_help(actions::print_help);
}

Program &Program::auto_help(actions::signature<ArgumentType::FLAG> action) noexcept {
  this->flag("help", "h").help("Display this information").action(action);
  this->has_auto_help = true;
  return *this;
}

Positional &Program::pos(std::string_view name) {
  Positional arg{.name = name, .is_required = true};
  return *positionals.insert(positionals.end(), arg);
}

Option &Program::opt(std::string_view name) { return opt(name, {}); }

Option &Program::opt(std::string_view name, char const (&abbrev)[2]) {
  auto const idx = options.size();
  auto &opt = options.emplace_back(name, abbrev);
  options_idx[opt.name] = idx;
  if (opt.has_abbrev())
    options_idx[opt.abbrev] = idx;
  return opt;
}

Flag &Program::flag(std::string_view name) { return flag(name, {}); }

Flag &Program::flag(std::string_view name, char const (&abbrev)[2]) {
  auto const idx = flags.size();
  Flag arg{.name = name, .abbrev = abbrev, .default_value = false, .set_value = true, .act = actions::assign<bool>};
  auto &flag = *flags.insert(flags.end(), arg);
  flags_idx[flag.name] = idx;
  if (flag.has_abbrev())
    flags_idx[flag.abbrev] = idx;
  return flag;
}

Program &Program::cmd(std::string_view name) {
  if (cmds_idx.contains(name)) {
    throw ArgumentAlreadyExists(name);
  }
  auto const idx = cmds.size();
  auto &command = cmds.emplace_back(name, std::make_unique<Program>());
  cmds_idx[name] = idx;
  if (this->has_auto_help) {
    command.spec->auto_help(flags[0].act);
  }
  return *command.spec;
}

ArgMap Program::operator()(int argc, char const *argv[]) {
  return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
}

ArgMap Program::operator()(std::span<char const *> args) {
  try {
    this->path = args[0];
    auto map = parse(args);
    set_defaults(map);
    return map;
  } catch (UserError const &err) {
    std::exit(this->error_handler(*this, err));
  }
}

ArgMap Program::parse(std::span<char const *> args) const {
  ArgMap map;
  map.exec_path = std::string(args[0]);
  std::size_t current_positional_idx = 0;
  for (std::size_t index = 1; index < args.size();) {
    auto const arg = std::string_view(args[index]);
    if (is_dash_dash(arg)) {
      // +1 to ignore the dash-dash
      for (std::size_t offset = index + 1; offset < args.size();) {
        offset += assign_positional(map, args.subspan(offset), current_positional_idx);
        ++current_positional_idx;
      }
      index = args.size();
    } else if (auto cmd = is_command(arg); cmd != nullptr) {
      index += assign_command(map, args.subspan(index), *cmd);
    } else if (looks_positional(arg)) {
      index += assign_positional(map, args.subspan(index), current_positional_idx);
      ++current_positional_idx;
    } else if (auto const flags = is_short_flags(arg); !flags.empty()) {
      index += assign_many_flags(map, flags);
    } else if (auto const flag = is_long_flag(arg); !flag.empty()) {
      index += assign_flag(map, flag);
    } else if (auto const option = is_option(arg); option.has_value()) {
      index += assign_option(map, args.subspan(index), *option);
    } else {
      throw UnknownArgument(arg);
    }
  }
  return map;
}

void Program::set_defaults(ArgMap &map) const noexcept {
  using std::views::filter;
  using std::views::transform;
  auto wasnt_parsed = [&map](auto const &arg) { return !map.has(arg.name); };
  auto has_default = [](auto const &arg) { return arg.default_value.has_value(); };
  for (auto const &positional : positionals | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::POSITIONAL>(map, positional);
  for (auto const &flag : flags | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::FLAG>(map, flag);
  for (auto const &option : options | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::OPTION>(map, option);
}

// +-----------------+
// | parsing helpers |
// +-----------------+

bool Program::is_dash_dash(std::string_view const whole_arg) const noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

Command const *Program::is_command(std::string_view const whole_arg) const noexcept {
  if (auto const cmd_idx = cmds_idx.find(whole_arg); cmd_idx != cmds_idx.end())
    return &cmds[cmd_idx->second];
  return nullptr;
}

bool Program::looks_positional(std::string_view const whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  return num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string_view::npos);
}

std::string_view Program::is_short_flags(std::string_view const whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  auto const all_short_flags =
      std::all_of(flags.begin(), flags.end(), [this](char const c) { return this->is_flag(std::string_view(&c, 1)); });
  if (num_of_dashes == 1 && flags.length() >= 1 && all_short_flags)
    return flags;
  return {};
}

std::string_view Program::is_long_flag(std::string_view const whole_arg) const noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2 && is_flag(name))
    return name;
  return {};
}

std::optional<ParsedOption> Program::is_option(std::string_view const whole_arg) const noexcept {
  auto const parsed_option = parse_option(whole_arg);
  if (options_idx.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

bool Program::is_flag(std::string_view const name) const noexcept { return flags_idx.contains(name); }

// +---------------------+
// | parsing assignments |
// +---------------------+

std::size_t Program::assign_command(ArgMap &map, std::span<char const *> args, Command const &cmd) const {
  auto const exec_path = fmt::format("{} {}", this->path, cmd.name);
  args[0] = exec_path.data();
  map.cmd_name = cmd.name;
  map.cmd_args = memory::ValuePtr(std::make_unique<ArgMap>(std::move((*cmd.spec)(args))));
  return args.size();
}

std::size_t Program::assign_positional(ArgMap &map, std::span<char const *> args,
                                       std::size_t const positional_idx) const {
  if (positional_idx >= this->positionals.size()) {
    throw UnexpectedPositional(args[0], this->positionals.size());
  }
  auto const arg = this->positionals[positional_idx];
  // if gather amount is 0, we gather everything else
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() : arg.gather_n.amount;
  if (gather_amount > args.size()) {
    throw MissingValue(arg.name, gather_amount, args.size());
  }
  for (std::size_t count = 0; count < gather_amount; ++count) {
    arg.act(*this, map, arg, args[count]);
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
  auto const arg_idx = this->flags_idx.at(flag);
  auto const arg = this->flags[arg_idx];
  arg.act(*this, map, arg, std::nullopt);
  return 1;
}

std::size_t Program::assign_option(ArgMap &map, std::span<char const *> args, ParsedOption const option) const {
  auto const arg_idx = this->options_idx.at(option.name);
  auto const arg = this->options[arg_idx];
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - 1 : arg.gather_n.amount;
  if (option.value) {
    if (gather_amount != 1) {
      throw MissingValue(arg.name, gather_amount, 1);
    }
    arg.act(*this, map, arg, *option.value);
    return 1;
  } else if (gather_amount < args.size()) {
    // + 1 everywhere because 0 is the index in `args` that the option is,
    // so 1 is where we start to parse values up to gather amount
    std::size_t count = 0;
    do {
      auto const value = std::string_view(args[count + 1]);
      arg.act(*this, map, arg, value);
      ++count;
    } while (count < gather_amount);
    return gather_amount + 1;
  } else if (arg.set_value) {
    arg.act(*this, map, arg, std::nullopt);
    return 1;
  } else {
    throw MissingValue(arg.name, gather_amount, args.size() - 1);
  }
}

ParsedOption parse_option(std::string_view const whole_arg) noexcept {
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

HelpFormatter::HelpFormatter(Program const &program, std::ostream &out)
    : out(out), max_width(program.msg_width), program_title(program.title), program_introduction(program.introduction),
      program_description(program.description), program_path(program.path), flags(program.flags),
      options(program.options), positionals(program.positionals), cmds(program.cmds) {
  std::sort(flags.begin(), flags.end());
  std::sort(options.begin(), options.end());
  std::sort(positionals.begin(), positionals.end());
  std::sort(cmds.begin(), cmds.end());
}

std::size_t HelpFormatter::help_padding_size() const noexcept {
  using std::views::transform;
  std::array<std::size_t, 3> lengths{0, 0, 0};
  auto const get_name = [](auto const &arg) -> std::string_view { return arg.name; };
  if (!flags.empty())
    lengths[0] = std::ranges::max(flags | transform(get_name) | transform(&std::string_view::length));
  if (!options.empty())
    lengths[1] = std::ranges::max(options | transform(get_name) | transform(&std::string_view::length));
  if (!positionals.empty())
    lengths[2] = std::ranges::max(positionals | transform(get_name) | transform(&std::string_view::length));
  return 3 * std::ranges::max(lengths);
}

void HelpFormatter::print_title() const noexcept {
  if (!program_title.empty())
    out << program_title << nl;
  else
    out << program_path << nl;
}

void HelpFormatter::print_intro() const noexcept {
  if (program_introduction.length() <= max_width) {
    out << program_introduction << nl;
  } else {
    out << limit_string_within(program_introduction, max_width) << nl;
  }
}

void HelpFormatter::print_long_usage() const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::transform;
  using std::views::drop, std::views::take;

  std::vector<std::string> words;
  words.reserve(1 + positionals.size() + options.size() + flags.size() + cmds.size());

  auto insert = std::back_inserter(words);
  words.push_back(program_path);
  transform(positionals, insert, &Positional::format_usage);
  transform(options, insert, &Option::format_usage);
  transform(flags, insert, &Flag::format_usage);

  if (cmds.size() == 1) {
    words.push_back(format("{{{}}}", cmds.front().name));
  } else if (cmds.size() > 1) {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", cmds.front().name));
    transform(cmds | drop(1) | take(cmds.size() - 2), insert, [](auto const &cmd) { return format("{},", cmd.name); });
    words.push_back(format("{}}}", cmds.back().name));
  }

  // -4 because we'll later print a left margin of 4 spaces
  auto const split_lines = limit_within(words, max_width - 4);
  out << "Usage:\n";
  for (auto const &line : split_lines) {
    out << format("    {}\n", join(line, " "));
  }
}

void HelpFormatter::print_help() const noexcept {
  std::string_view pending_nl = "";
  auto const padding = std::string(help_padding_size(), ' ');

  if (!positionals.empty()) {
    out << "Positionals:\n";
    for (auto const &arg : positionals) {
      print_arg_help(arg, padding);
    }
    pending_nl = "\n";
  }

  if (!options.empty()) {
    out << pending_nl << "Options:\n";
    for (auto const &arg : options) {
      print_arg_help(arg, padding);
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!flags.empty()) {
    out << pending_nl << "Flags:\n";
    for (auto const &arg : flags) {
      print_arg_help(arg, padding);
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!cmds.empty()) {
    out << pending_nl << "Commands:\n";
    for (auto const &arg : cmds) {
      print_arg_help(arg, padding);
    }
  }
}

void HelpFormatter::print_description() const noexcept {
  if (program_description.empty())
    return;
  if (program_description.length() <= max_width)
    out << program_description << nl;
  else
    out << limit_string_within(program_description, max_width) << nl;
}

// +-----------+
// | utilities |
// +-----------+

void print_full_help(Program const &program, std::ostream &ostream) noexcept {
  HelpFormatter formatter(program, ostream);
  formatter.print_title();
  if (!program.introduction.empty()) {
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

void print_help(Program const &program, ArgMap &, Flag const &, std::optional<std::string_view> const) {
  print_full_help(program);
  std::exit(0);
}

} // namespace actions

} // namespace opzioni
