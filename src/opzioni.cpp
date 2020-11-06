#include "opzioni.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <optional>
#include <tuple>
#include <variant>

namespace opzioni {

// +-----+
// | Arg |
// +-----+

template <> std::string Arg<ArgumentType::POSITIONAL>::format_usage() const noexcept {
  return fmt::format("<{}>", name);
}

template <> std::string Arg<ArgumentType::POSITIONAL>::format_long_usage() const noexcept { return name; }

template <> std::string Arg<ArgumentType::OPTION>::format_usage() const noexcept {
  // width of: at least one dash + another dash if not single letter + name length
  int const width = 1 + int(name.length() > 1) + name.length();
  if (this->is_required)
    return fmt::format("{0:->{1}} <{0}>", name, width);
  return fmt::format("[{0:->{1}} <{0}>]", name, width);
}

template <> std::string Arg<ArgumentType::OPTION>::format_long_usage() const noexcept {
  if (has_abbrev())
    return fmt::format("-{0}, --{1} <{0}>", abbrev, name);
  // width of: at least one dash + another dash if not single letter + name length
  int const width = 1 + int(name.length() > 1) + name.length();
  return fmt::format("{0:->{1}} <{0}>", name, width);
}

template <> std::string Arg<ArgumentType::FLAG>::format_usage() const noexcept {
  // width of: at least one dash + another dash if not single letter + name length
  int const width = 1 + int(name.length() > 1) + name.length();
  if (this->is_required)
    return fmt::format("{0:->{1}}", name, width);
  return fmt::format("[{0:->{1}}]", name, width);
}

template <> std::string Arg<ArgumentType::FLAG>::format_long_usage() const noexcept {
  if (has_abbrev())
    return fmt::format("-{}, --{}", abbrev, name);
  // width of: at least one dash + another dash if not single letter + name length
  int const width = 1 + int(name.length() > 1) + name.length();
  return fmt::format("{0:->{1}}", name, width);
}

template <> std::string Arg<ArgumentType::POSITIONAL>::format_description() const noexcept {
  return fmt::format("{}", description);
}

template <> std::string Arg<ArgumentType::OPTION>::format_description() const noexcept {
  return fmt::format("{}", description);
}

template <> std::string Arg<ArgumentType::FLAG>::format_description() const noexcept {
  return fmt::format("{}", description);
  // auto const fmt_variant = [](auto const &var) { return fmt::format("{}", var); };
  // auto format =
  //     fmt::format("{}: {} (sets {} if present", format_usage(), description, std::visit(fmt_variant, *set_value));
  // if (default_value) {
  //   format += fmt::format(", default is {})", std::visit(fmt_variant, *default_value));
  // } else {
  //   format += fmt::format(")");
  // }
  // return format;
}

// +---------+
// | Program |
// +---------+

Program &Program::help(std::string epilog) noexcept {
  this->epilog = epilog;
  return *this;
}

Program &Program::details(std::string description) noexcept {
  this->description = description;
  return *this;
}

Positional &Program::pos(std::string name) {
  Positional arg{.name = name, .is_required = true};
  return *positionals.insert(positionals.end(), arg);
}

Option &Program::opt(std::string name) { return opt(name, '\0'); }

Option &Program::opt(std::string name, char abbrev) {
  auto const idx = options.size();
  auto &opt = options.emplace_back(name, abbrev);
  options_idx[opt.name] = idx;
  if (opt.has_abbrev())
    options_idx[std::string(1, opt.abbrev)] = idx;
  return opt;
}

Flag &Program::flag(std::string name) { return flag(name, '\0'); }

Flag &Program::flag(std::string name, char abbrev) {
  auto const idx = flags.size();
  Flag arg{.name = name, .abbrev = abbrev, .set_value = true, .act = actions::assign<bool>};
  auto &flag = *flags.insert(flags.end(), arg);
  flags_idx[flag.name] = idx;
  if (flag.has_abbrev())
    flags_idx[std::string(1, flag.abbrev)] = idx;
  return flag;
}

Program &Program::cmd(std::string name) {
  if (cmds_idx.contains(name)) {
    throw ArgumentAlreadyExists(fmt::format("Subcommand `{}` already exists.", name));
  }
  auto const idx = cmds.size();
  auto &command = cmds.emplace_back(std::make_unique<Program>(name));
  cmds_idx[command->name] = idx;
  return *command;
}

ArgMap Program::operator()(int argc, char const *argv[]) {
  return (*this)(std::span<char const *>{argv, static_cast<std::size_t>(argc)});
}

ArgMap Program::operator()(std::span<char const *> args) {
  parsing::Parser parser(*this, args);
  auto map = parser();
  set_defaults(map);
  return map;
}

std::string Program::format_long_usage() const noexcept { return name; }

std::string Program::format_description() const noexcept { return epilog; }

void Program::print_usage(std::ostream &ostream) const noexcept {
  HelpFormatter formatter(*this, 80);
  ostream << formatter.title();
  ostream << nl;
  ostream << formatter.usage();
  ostream << nl;
  ostream << formatter.help();
  ostream << nl;
  ostream << formatter.description();
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

Program *Program::is_command(std::string const &whole_arg) const noexcept {
  if (auto const cmd_idx = cmds_idx.find(whole_arg); cmd_idx != cmds_idx.end())
    return cmds[cmd_idx->second].get();
  return nullptr;
}

bool Program::is_flag(std::string const &name) const noexcept { return flags_idx.contains(name); }

std::optional<std::string> Program::is_long_flag(std::string const &whole_arg) const noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2 && is_flag(name))
    return name;
  return std::nullopt;
}

std::optional<std::string> Program::is_short_flags(std::string const &whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  auto const all_short_flags =
      std::all_of(flags.begin(), flags.end(), [this](char const &c) { return this->is_flag(std::string(1, c)); });
  if (num_of_dashes == 1 && flags.length() >= 1 && all_short_flags)
    return flags;
  return std::nullopt;
}

std::optional<parsing::ParsedOption> Program::is_option(std::string const &whole_arg) const noexcept {
  auto const parsed_option = parsing::parse_option(whole_arg);
  if (options_idx.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

// +------------+
// | formatting |
// +------------+

HelpFormatter::HelpFormatter(Program const &program, std::size_t const max_width)
    : max_width(max_width), program_name(program.name), program_description(program.description),
      program_epilog(program.epilog), flags(program.flags), options(program.options), positionals(program.positionals),
      cmds(program.cmds) {
  std::sort(flags.begin(), flags.end());
  std::sort(options.begin(), options.end());
  std::sort(positionals.begin(), positionals.end());
  std::sort(cmds.begin(), cmds.end(), [](auto const &lhs, auto const &rhs) { return *lhs < *rhs; });
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



std::string HelpFormatter::title() const noexcept {
  if (program_epilog.empty()) {
    return fmt::format("{}.\n", program_name);
  } else {
    return fmt::format("{} -- {}.\n", program_name, program_epilog);
  }
}

std::string HelpFormatter::usage() const noexcept {
  using fmt::format, fmt::join;
  using std::views::drop, std::views::take, std::views::transform;

  std::vector<std::string> words;
  words.reserve(1 + positionals.size() + options.size() + flags.size() + cmds.size());

  auto insert = std::back_inserter(words);
  words.push_back(program_name);
  std::ranges::transform(positionals, insert, &Positional::format_usage);
  std::ranges::transform(options, insert, &Option::format_usage);
  std::ranges::transform(flags, insert, &Flag::format_usage);

  if (cmds.size() == 1) {
    words.push_back(format("{{{}}}", cmds.front()->name));
  } else {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", cmds.front()->name));
    std::ranges::transform(cmds | drop(1) | take(cmds.size() - 2), insert,
                           [](auto const &cmd) { return format("{},", cmd->name); });
    words.push_back(format("{}}}", cmds.back()->name));
  }

  auto const split_lines = limit_within(words, this->max_width, 4);
  auto const lines = split_lines | transform([](auto const &line) { return format("    {}", join(line, " ")); });
  return format("Usage:\n{}\n", join(lines, "\n"));
}

std::string HelpFormatter::help() const noexcept {
  auto const padding_size = help_padding_size();

  std::vector<std::string> help_parts;
  help_parts.reserve(4);

  if (!positionals.empty()) {
    help_parts.emplace_back(fmt::format("Positionals:\n{}\n", format_help(positionals, padding_size)));
  }

  if (!options.empty()) {
    help_parts.emplace_back(fmt::format("Options:\n{}\n", format_help(options, padding_size)));
  }

  if (!flags.empty()) {
    help_parts.emplace_back(fmt::format("Flags:\n{}\n", format_help(flags, padding_size)));
  }

  if (!cmds.empty()) {
    auto const cmd_refs = cmds | std::views::transform([](auto const &cmd) -> Program const & { return *cmd; });
    help_parts.emplace_back(fmt::format("Commands:\n{}\n", format_help(cmd_refs, padding_size)));
  }

  return fmt::format("{}", fmt::join(help_parts, "\n"));
}

std::string HelpFormatter::description() const noexcept {
  if (program_description.empty())
    return "";
  using fmt::join;
  using std::views::transform;
  auto const lines = limit_within(program_description, this->max_width);
  return fmt::format("{}.\n", join(lines | transform([](auto const &words) { return join(words, " "); }), "\n"));
}

// +---------+
// | parsing |
// +---------+

namespace parsing {

ArgMap Parser::operator()() {
  map.reset();
  map.cmd_name = std::string(args[0]);
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
  auto char_to_flag = [](char c) { return Flag{std::string(1, c)}; };
  for (auto const &flag : flags.chars | transform(char_to_flag)) {
    (*this)(flag);
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
      auto const value = std::string(args[option.index + 1 + count]);
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
    throw UnknownArgument(
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
  auto subargs = args.last(remaining_args_count);
  map.subcmd = memory::ValuePtr(std::make_unique<ArgMap>(std::move(cmd.program(subargs))));
  return remaining_args_count;
}

std::size_t Parser::operator()(Unknown unknown) { throw UnknownArgument(std::string(args[unknown.index])); }

alternatives Parser::decide_type(std::size_t index) const noexcept {
  auto const arg = std::string(args[index]);

  if (is_dash_dash(arg))
    return DashDash{index};

  if (auto cmd = spec.is_command(arg); cmd != nullptr)
    return Command{*cmd, index};

  if (auto const positional = looks_positional(arg); positional)
    return Positional{index};

  if (auto const flags = spec.is_short_flags(arg); flags)
    return ManyFlags{*flags};

  if (auto const flag = spec.is_long_flag(arg); flag)
    return Flag{*flag};

  if (auto const option = spec.is_option(arg); option)
    return Option{*option, index};

  return Unknown{index};
}

// +-----------+
// | "helpers" |
// +-----------+

bool Parser::is_dash_dash(std::string const &whole_arg) const noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

std::optional<std::string> Parser::looks_positional(std::string const &whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string::npos))
    return whole_arg;
  return std::nullopt;
}

bool Parser::would_be_positional(std::size_t idx) const noexcept {
  return std::holds_alternative<Positional>(decide_type(idx));
}

ParsedOption parse_option(std::string whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  auto const eq_idx = whole_arg.find('=', num_of_dashes);
  bool const has_equals = eq_idx != std::string::npos;
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

void print_help(Program const &program, ArgMap &, Flag const &, std::optional<std::string> const &) {
  program.print_usage();
  std::exit(0);
}

} // namespace actions

} // namespace opzioni
