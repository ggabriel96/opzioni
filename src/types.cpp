#include "types.hpp"
#include "parsing.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

#include <fmt/format.h>

namespace opzioni {

Arg &Arg::help(std::string description) noexcept {
  this->description = description;
  return *this;
}

Arg &Arg::required() noexcept {
  this->is_required = true;
  return *this;
}

Arg &Arg::action(actions::signature action_fn) noexcept {
  this->action_fn = action_fn;
  return *this;
}

Program &Program::help(std::string description) noexcept {
  this->description = description;
  return *this;
}

Program &Program::with_epilog(std::string epilog) noexcept {
  this->epilog = epilog;
  return *this;
}

Arg &Program::pos(std::string name) {
  Arg arg{name};
  return *positional_args.insert(positional_args.end(), arg);
}

Arg &Program::opt(std::string name) {
  Arg arg{name};
  return options[arg.name] = arg;
}

Arg &Program::flag(std::string name) {
  Arg arg{name};
  return flags[arg.name] = arg;
}

Program &Program::cmd(std::string name) {
  if (cmds.contains(name)) {
    throw ArgumentAlreadyExists(fmt::format("Subcommand `{}` already exists.", name));
  }
  auto &cmd = cmds[name] = memory::ValuePtr(std::make_unique<Program>(name));
  return *cmd;
}

std::optional<decltype(Program::cmds)::const_iterator> Program::is_subcmd(std::string const &name) const noexcept {
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
  auto const parsed_option = parsing::parse_option(whole_arg);
  if (options.contains(parsed_option.name))
    return parsed_option;
  return std::nullopt;
}

ArgMap Program::operator()(int argc, char const *argv[]) const {
  std::span<char const *> args{argv, static_cast<std::size_t>(argc)};
  parsing::ArgumentParser parser(*this, args);
  return parser();
}

} // namespace opzioni
