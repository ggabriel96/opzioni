#include "program.hpp"
#include "parsing.hpp"

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

ArgMap Program::operator()(int argc, char const *argv[]) const {
  std::span<char const *> args{argv, static_cast<std::size_t>(argc)};
  parsing::ArgumentParser parser(*this, args);
  auto map = parser();
  set_defaults(map);
  return map;
}

// +-------------------+
// | "parsing helpers" |
// +-------------------+

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

std::optional<decltype(Program::cmds)::const_iterator> Program::is_command(std::string const &name) const noexcept {
  if (auto const cmd = cmds.find(name); cmd != cmds.end())
    return cmd;
  return std::nullopt;
}

std::optional<std::string> Program::is_positional(std::string const &whole_arg) const noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string::npos))
    return whole_arg;
  return std::nullopt;
}

bool Program::is_flag(std::string const &name) const noexcept { return flags.contains(name); }

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

std::optional<ParsedOption> Program::is_option(std::string const &whole_arg) const noexcept {
  auto const parsed_option = parse_option(whole_arg);
  if (options.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

ParsedOption parse_option(std::string const &whole_arg) noexcept {
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

} // namespace opzioni
