#include "program.hpp"

#include <optional>
#include <ranges>

namespace opzioni {

// +-------------------------------+
// | directly related to user spec |
// +-------------------------------+

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
  Option arg{name};
  return options[arg.name] = arg;
}

Flag &Program::flag(std::string name) {
  Flag arg{.name = name, .set_value = true, .act = actions::assign<bool>};
  return flags[arg.name] = arg;
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
  map.reset();
  this->args = args;
  map.cmd_name = std::string(args[0]);
  current_positional_idx = 0;
  for (std::size_t index = 1; index < args.size();) {
    index += std::visit(*this, decide_type(index));
  }
  return map;
}

// +---------+
// | parsing |
// +---------+

bool Program::is_flag(std::string const &name) const noexcept { return flags.contains(name); }

void Program::set_defaults(ArgMap &map) const noexcept {
  using std::views::filter;
  using std::views::transform;
  auto pair_to_arg = [](auto const &pair) { return pair.second; };
  auto wasnt_parsed = [&map](auto const &arg) { return !map.has(arg.name); };
  auto has_default = [](auto const &arg) { return arg.default_value.has_value(); };
  for (auto const &positional : positionals | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::POSITIONAL>(map, positional);
  for (auto const &flag : flags | transform(pair_to_arg) | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::FLAG>(map, flag);
  for (auto const &option : options | transform(pair_to_arg) | filter(wasnt_parsed) | filter(has_default))
    set_default<ArgumentType::OPTION>(map, option);
}

std::size_t Program::operator()(parsing::DashDash dd) {
  std::size_t const next_args_count = args.size() - (dd.index + 1);
  for (std::size_t i = dd.index + 1; i < args.size();) {
    i += (*this)(parsing::Positional{i});
  }
  return 1 + next_args_count; // +1 because we also count the dash-dash
}

std::size_t Program::operator()(parsing::Flag flag) {
  auto const arg = flags.at(flag.name);
  arg.act(map, arg, std::nullopt);
  return 1;
}

std::size_t Program::operator()(parsing::ManyFlags flags) {
  using std::views::transform;
  auto char_to_flag = [](char c) { return parsing::Flag{std::string(1, c)}; };
  for (auto const &flag : flags.chars | transform(char_to_flag)) {
    (*this)(flag);
  }
  return 1;
}

std::size_t Program::operator()(parsing::Option option) {
  auto const arg = options.at(option.arg.name);
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - option.index : arg.gather_n.amount;
  if (option.arg.value) {
    if (gather_amount != 1) {
      throw MissingValue(fmt::format("Expected {} values for option `{}`, got 1", gather_amount, arg.name));
    }
    arg.act(map, arg, *option.arg.value);
    return 1;
  } else if (option.index + 1 + gather_amount <= args.size() &&
             std::holds_alternative<parsing::Positional>(decide_type(option.index + 1))) {
    // + 1 everywhere because `option.index` is the index in `args` that the option is.
    // From that index + 1 is where we start to parse values up to gather amount
    std::size_t count = 0;
    do {
      auto const value = std::string(args[option.index + 1 + count]);
      arg.act(map, arg, value);
      ++count;
    } while (count < gather_amount &&
             std::holds_alternative<parsing::Positional>(decide_type(option.index + 1 + count)));
    return 1 + count;
  } else if (arg.set_value) {
    arg.act(map, arg, std::nullopt);
    return 1;
  } else {
    throw MissingValue(fmt::format("Expected {} values for option `{}`, got {}", gather_amount, arg.name,
                                   args.size() - (option.index + 1)));
  }
}

std::size_t Program::operator()(parsing::Positional positional) {
  if (current_positional_idx >= positionals.size()) {
    throw UnknownArgument(
        fmt::format("Unexpected positional argument `{}`. This program expects {} positional arguments",
                    args[positional.index], positionals.size()));
  }
  auto const arg = positionals[current_positional_idx];
  // if gather amount is 0, we gather everything else
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - positional.index : arg.gather_n.amount;
  if (positional.index + gather_amount > args.size()) {
    throw MissingValue(fmt::format("Expected {} values for positional argument `{}`, got {}", gather_amount, arg.name,
                                   args.size() - positional.index));
  }
  for (std::size_t count = 0; count < gather_amount; ++count) {
    arg.act(map, arg, args[positional.index + count]);
  }
  ++current_positional_idx;
  return gather_amount;
}

std::size_t Program::operator()(parsing::Command cmd) {
  auto const remaining_args_count = args.size() - cmd.index;
  auto subargs = args.last(remaining_args_count);
  map.subcmd = memory::ValuePtr(std::make_unique<ArgMap>(std::move(cmd.program(subargs))));
  return remaining_args_count;
}

std::size_t Program::operator()(parsing::Unknown unknown) { throw UnknownArgument(std::string(args[unknown.index])); }

parsing::alternatives Program::decide_type(std::size_t index) const noexcept {
  auto const arg = std::string(args[index]);

  if (is_dash_dash(arg))
    return parsing::DashDash{index};

  if (auto cmd = is_command(arg); cmd != nullptr)
    return parsing::Command{*cmd, index};

  if (auto const positional = is_positional(arg); positional)
    return parsing::Positional{index};

  if (auto const flags = is_short_flags(arg); flags)
    return parsing::ManyFlags{*flags};

  if (auto const flag = is_long_flag(arg); flag)
    return parsing::Flag{*flag};

  if (auto const option = is_option(arg); option)
    return parsing::Option{*option, index};

  return parsing::Unknown{index};
}

// +-----------+
// | "helpers" |
// +-----------+

bool Program::is_dash_dash(std::string const &whole_arg) const noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

Program *Program::is_command(std::string const &whole_arg) const noexcept {
  if (auto const cmd = cmds.find(whole_arg); cmd != cmds.end())
    return cmd->second.get();
  return nullptr;
}

std::optional<std::string> Program::is_positional(std::string const &whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string::npos))
    return whole_arg;
  return std::nullopt;
}

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
  if (options.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

namespace parsing {

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

} // namespace opzioni
