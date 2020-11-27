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
  std::cerr << limit_string_within(err.what(), 80) << nl;
  return -1;
}

int print_error_and_usage(Program const &program, UserError const &err) noexcept {
  print_error(program, err);
  HelpFormatter formatter(program, 80, std::cerr);
  std::cerr << nl;
  formatter.print_long_usage();
  std::cerr << "\nSee `" << program.path << " --help` for more information\n";
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

Program &Program::override_help(actions::signature<ArgumentType::FLAG> action) noexcept {
  auto const help_idx = flags_idx["help"];
  auto &help = flags[help_idx];
  help.action(action);
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
  Flag arg{.name = name, .abbrev = abbrev, .set_value = true, .act = actions::assign<bool>};
  auto &flag = *flags.insert(flags.end(), arg);
  flags_idx[flag.name] = idx;
  if (flag.has_abbrev())
    flags_idx[flag.abbrev] = idx;
  return flag;
}

Program &Program::cmd(std::string_view name) {
  if (cmds_idx.contains(name)) {
    throw ArgumentAlreadyExists(fmt::format("Subcommand `{}` already exists.", name));
  }
  auto const idx = cmds.size();
  auto &command = cmds.emplace_back(name, std::make_unique<Program>());
  cmds_idx[name] = idx;
  return *command.spec;
}

ArgMap Program::operator()(int argc, char const *argv[]) {
  return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
}

ArgMap Program::operator()(std::span<char const *> args) {
  try {
    this->path = args[0];
    parsing::Parser parser(*this, args);
    auto map = parser();
    set_defaults(map);
    return map;
  } catch (UserError const &err) {
    std::exit(this->error_handler(*this, err));
  }
}

void Program::print_usage(std::size_t const max_width, std::ostream &ostream) const noexcept {
  HelpFormatter formatter(*this, max_width, ostream);
  formatter.print_title();
  if (!introduction.empty()) {
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

Command const *Program::is_command(std::string_view const whole_arg) const noexcept {
  if (auto const cmd_idx = cmds_idx.find(whole_arg); cmd_idx != cmds_idx.end())
    return &cmds[cmd_idx->second];
  return nullptr;
}

bool Program::is_flag(std::string_view const name) const noexcept { return flags_idx.contains(name); }

std::string_view Program::is_long_flag(std::string_view const whole_arg) const noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2 && is_flag(name))
    return name;
  return {};
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

std::optional<parsing::ParsedOption> Program::is_option(std::string_view const whole_arg) const noexcept {
  auto const parsed_option = parsing::parse_option(whole_arg);
  if (options_idx.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

// +------------+
// | formatting |
// +------------+

HelpFormatter::HelpFormatter(Program const &program, std::size_t const max_width, std::ostream &out)
    : out(out), max_width(max_width), program_title(program.title), program_introduction(program.introduction),
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

Program program(std::string_view title) noexcept {
  Program program(title);
  program.flag("help", "h").help("Display this information").action(actions::print_help);
  program.error_handler = print_error_and_usage;
  return program;
}

// +---------+
// | parsing |
// +---------+

namespace parsing {

ArgMap Parser::operator()() {
  map.reset();
  map.exec_path = std::string(args[0]);
  current_positional_idx = 0;
  for (std::size_t index = 1; index < args.size();) {
    index += std::visit(*this, decide_type(index));
  }
  return map;
}

std::size_t Parser::operator()(DashDash dd) {
  std::size_t const next_args_count = args.size() - (dd.index + 1);
  for (std::size_t i = dd.index + 1; i < args.size();) {
    i += (*this)(Positional{i});
  }
  return 1 + next_args_count; // +1 because we also count the dash-dash
}

std::size_t Parser::operator()(Flag flag) {
  auto const arg_idx = spec.flags_idx.at(flag.name);
  auto const arg = spec.flags[arg_idx];
  arg.act(spec, map, arg, std::nullopt);
  return 1;
}

std::size_t Parser::operator()(ManyFlags flags) {
  using std::views::transform;
  for (auto const flag : flags.chars) {
    (*this)(Flag{std::string_view(&flag, 1)});
  }
  return 1;
}

std::size_t Parser::operator()(Option option) {
  auto const arg_idx = spec.options_idx.at(option.arg.name);
  auto const arg = spec.options[arg_idx];
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - option.index : arg.gather_n.amount;
  if (option.arg.value) {
    if (gather_amount != 1) {
      throw MissingValue(fmt::format("Expected {} values for option `{}`, got 1", gather_amount, arg.name));
    }
    arg.act(spec, map, arg, *option.arg.value);
    return 1;
  } else if (option.index + 1 + gather_amount <= args.size() && would_be_positional(option.index + 1)) {
    // + 1 everywhere because `option.index` is the index in `args` that the option is.
    // From that index + 1 is where we start to parse values up to gather amount
    std::size_t count = 0;
    do {
      auto const value = std::string_view(args[option.index + 1 + count]);
      arg.act(spec, map, arg, value);
      ++count;
    } while (count < gather_amount && would_be_positional(option.index + 1 + count));
    return 1 + count;
  } else if (arg.set_value) {
    arg.act(spec, map, arg, std::nullopt);
    return 1;
  } else {
    throw MissingValue(fmt::format("Expected {} values for option `{}`, got {}", gather_amount, arg.name,
                                   args.size() - (option.index + 1)));
  }
}

std::size_t Parser::operator()(Positional positional) {
  if (current_positional_idx >= spec.positionals.size()) {
    throw ParseError(
        fmt::format("Unexpected positional argument `{}`. This program expects {} positional arguments",
                    args[positional.index], spec.positionals.size()));
  }
  auto const arg = spec.positionals[current_positional_idx];
  // if gather amount is 0, we gather everything else
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - positional.index : arg.gather_n.amount;
  if (positional.index + gather_amount > args.size()) {
    throw MissingValue(fmt::format("Expected {} values for positional argument `{}`, got {}", gather_amount, arg.name,
                                   args.size() - positional.index));
  }
  for (std::size_t count = 0; count < gather_amount; ++count) {
    arg.act(spec, map, arg, args[positional.index + count]);
  }
  ++current_positional_idx;
  return gather_amount;
}

std::size_t Parser::operator()(Command cmd) {
  auto const remaining_args_count = args.size() - cmd.index;
  auto const exec_path = fmt::format("{} {}", spec.path, cmd.name);
  auto subargs = args.last(remaining_args_count);
  subargs[0] = exec_path.data();
  map.cmd_name = cmd.name;
  map.cmd_args = memory::ValuePtr(std::make_unique<ArgMap>(std::move(cmd.spec(subargs))));
  return remaining_args_count;
}

std::size_t Parser::operator()(Unknown unknown) { throw UnknownArgument(std::string(args[unknown.index])); }

alternatives Parser::decide_type(std::size_t index) const noexcept {
  auto const arg = std::string_view(args[index]);

  if (is_dash_dash(arg))
    return DashDash{index};

  if (auto cmd = spec.is_command(arg); cmd != nullptr)
    return Command{index, cmd->name, *cmd->spec};

  if (auto const positional = looks_positional(arg); !positional.empty())
    return Positional{index};

  if (auto const flags = spec.is_short_flags(arg); !flags.empty())
    return ManyFlags{flags};

  if (auto const flag = spec.is_long_flag(arg); !flag.empty())
    return Flag{flag};

  if (auto const option = spec.is_option(arg); option)
    return Option{*option, index};

  return Unknown{index};
}

// +-----------+
// | "helpers" |
// +-----------+

bool Parser::is_dash_dash(std::string_view const whole_arg) const noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

std::string_view Parser::looks_positional(std::string_view const whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string_view::npos))
    return whole_arg;
  return {};
}

bool Parser::would_be_positional(std::size_t idx) const noexcept {
  return std::holds_alternative<Positional>(decide_type(idx));
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

} // namespace parsing

namespace actions {

void print_help(Program const &program, ArgMap &, Flag const &, std::optional<std::string_view> const) {
  program.print_usage();
  std::exit(0);
}

} // namespace actions

} // namespace opzioni
