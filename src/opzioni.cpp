#include "opzioni.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <tuple>
#include <variant>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opzioni {

template <> std::string Arg<ArgumentType::POSITIONAL>::format_usage() const noexcept {
  return fmt::format("<{}>", name);
}

template <> std::string Arg<ArgumentType::OPTION>::format_usage() const noexcept {
  if (this->is_required)
    return fmt::format("--{0} <{0}>", name);
  return fmt::format("[--{0} <{0}>]", name);
}

template <> std::string Arg<ArgumentType::FLAG>::format_usage() const noexcept {
  if (this->is_required)
    return fmt::format("--{}", name);
  return fmt::format("[--{}]", name);
}

template <> std::string Arg<ArgumentType::POSITIONAL>::format_long_usage() const noexcept { return format_usage(); }

template <> std::string Arg<ArgumentType::OPTION>::format_long_usage() const noexcept { return format_usage(); }

template <> std::string Arg<ArgumentType::FLAG>::format_long_usage() const noexcept { return format_usage(); }

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

Program &Program::help(std::string description) noexcept {
  this->description = description;
  return *this;
}

Program &Program::with_epilog(std::string epilog) noexcept {
  this->epilog = epilog;
  return *this;
}

Positional &Program::pos(std::string name) {
  Positional arg{.name = name, .is_required = true};
  return *positionals.insert(positionals.end(), arg);
}

Option &Program::opt(std::string name) {
  auto &opt = options.emplace_back(name);
  options_idx[opt.name] = options.size() - 1;
  return opt;
}

Flag &Program::flag(std::string name) {
  Flag arg{.name = name, .set_value = true, .act = actions::assign<bool>};
  auto &flag = *flags.insert(flags.end(), arg);
  flags_idx[flag.name] = flags.size() - 1;
  return flag;
}

Program &Program::cmd(std::string name) {
  if (cmds.contains(name)) {
    throw ArgumentAlreadyExists(fmt::format("Subcommand `{}` already exists.", name));
  }
  auto &cmd = cmds[name] = memory::ValuePtr(std::make_unique<Program>(name));
  return *cmd;
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

auto sorted_indices(auto const &range) {
  std::vector<std::size_t> indices(range.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::ranges::sort(indices, [&range](auto lhs, auto rhs) { return range[lhs] < range[rhs]; });
  return indices;
}

std::string format_help(auto const &range, std::size_t const margin) {
  using std::views::transform;
  auto const idx_to_elem = [&range](auto idx) { return range[idx]; };
  auto const format_elem = [margin](auto const &arg) {
    return fmt::format("{:>{}}: {}", arg.format_long_usage(), margin, arg.format_description());
  };
  auto const sorted_idx = sorted_indices(range);
  auto const formatted_range = sorted_idx | transform(idx_to_elem) | transform(format_elem);
  return fmt::format("{}", fmt::join(formatted_range, "\n"));
}

std::size_t Program::get_help_margin() const noexcept {
  auto const &longest_flag = std::ranges::max(flags, {}, [](auto const &arg) { return arg.name.length(); });
  auto const &longest_option = std::ranges::max(options, {}, [](auto const &arg) { return arg.name.length(); });
  auto const &longest_positional = std::ranges::max(positionals, {}, [](auto const &arg) { return arg.name.length(); });
  return 3 * std::max({longest_flag.name.length(), longest_option.name.length(), longest_positional.name.length()});
}

void Program::print_usage(std::ostream &ostream) const noexcept {
  ostream << format_title();

  ostream << format_usage();

  auto const margin = get_help_margin();

  ostream << "\nPositionals:\n";
  ostream << opzioni::format_help(positionals, margin) << nl;

  ostream << "\nFlags:\n";
  ostream << opzioni::format_help(flags, margin) << nl;

  ostream << "\nOptions:\n";
  ostream << opzioni::format_help(options, margin) << nl;

  if (!description.empty()) {
    ostream << nl << description << ".\n";
  }
}

std::string Program::format_title() const noexcept {
  if (epilog.empty()) {
    return fmt::format("{}.\n", name);
  } else {
    return fmt::format("{} -- {}.\n", name, epilog);
  }
}

std::string Program::format_usage() const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::sort, std::ranges::transform;
  using std::views::values;

  std::vector<std::string> positionals(this->positionals.size());
  transform(this->positionals, positionals.begin(), std::mem_fn(&Positional::format_usage));
  sort(positionals);

  std::vector<std::string> flags(this->flags.size());
  transform(this->flags, flags.begin(), std::mem_fn(&Flag::format_usage));
  sort(flags);

  std::vector<std::string> options(this->options.size());
  transform(this->options, options.begin(), std::mem_fn(&Option::format_usage));
  sort(options);

  auto const margin_size = 7 + name.length() + 1; // 7 == "Usage: ".length() + 1 space
  std::string const margin(margin_size, ' ');
  return format("\nUsage: {} {}\n{}{}\n{}{}\n", name, join(positionals, " "), margin, join(flags, " "), margin,
                join(options, " "));
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
  if (auto const cmd = cmds.find(whole_arg); cmd != cmds.end())
    return cmd->second.get();
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
  if (has_equals) {
    // long or short option with value
    auto const name = whole_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
    auto const value = whole_arg.substr(eq_idx + 1);
    return {name, value};
  } else if (num_of_dashes == 1 && whole_arg.length() > 2) {
    // has one dash, hence short option
    // but is longer than 2 characters and has no equals
    // hence short option with value (e.g. `-O2`)
    // (possibility of many short flags has already been tested for)
    auto const name = whole_arg.substr(1, 1);
    auto const value = whole_arg.substr(2);
    return {name, value};
  } else {
    // no equals and has 1 or 2 prefix dashes
    // hence option with next CLI argument as value
    auto const name = whole_arg.substr(num_of_dashes);
    return {.name = name, .value = std::nullopt};
  }
}

} // namespace parsing

namespace actions {

void print_help(Program const &program, ArgMap &, Flag const &, std::optional<std::string> const &) {
  program.print_usage();
  std::exit(0);
}

} // namespace actions

} // namespace opzioni
