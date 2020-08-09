#include "types.hpp"
#include "parsing.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

#include <fmt/format.h>

namespace opzioni {

void Program::pos(Arg &&arg) { this->positional_args.emplace_back(arg); }

void Program::opt(Arg &&arg) { this->options[arg.name] = arg; }

void Program::flag(Arg &&arg) { this->flags[arg.name] = arg; }

[[no_discard]] Program *Program::cmd(Command const &spec) {
  if (cmds.contains(spec.name)) {
    throw ArgumentAlreadyExists(fmt::format("Argument `{}` already exists.", spec.name));
  }
  auto result = cmds.insert({spec.name, std::make_unique<Program>(spec.name, spec.epilog, spec.description)});
  return result.first->second.get();
}

auto count_dashes(std::string_view cli_arg) { return cli_arg.find_first_not_of('-'); }

std::optional<std::string> Program::is_subcmd(std::string const &name) const noexcept {
  if (cmds.contains(name))
    return name;
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

Program const &Program::get_cmd(std::string name) const {
  auto const &subprogram = cmds.at(name);
  return *subprogram;
}

void Program::assign_positional_args(ArgMap &map, std::vector<std::string> const &parsed_positional) const {
  if (parsed_positional.size() < positional_args.size()) {
    auto const names = std::ranges::transform_view(positional_args, [](auto const &arg) { return arg.name; }) |
                       std::views::drop(parsed_positional.size());
    throw MissingRequiredArgument(fmt::format("Missing {} positional argument{:s<{}}: `{}`", names.size(), "",
                                              static_cast<int>(names.size() != 1), fmt::join(names, "`, `")));
  }
  if (parsed_positional.size() > positional_args.size()) {
    auto const args = std::ranges::drop_view(parsed_positional, positional_args.size());
    throw UnknownArgument(fmt::format("Got {} unexpected positional argument{:s<{}}: `{}`", args.size(), "",
                                      static_cast<int>(args.size() != 1), fmt::join(args, "`, `")));
  }
  for (size_t i = 0; i < positional_args.size(); ++i) {
    map.args[positional_args[i].name] = ArgValue{parsed_positional[i]};
  }
}

void Program::assign_flags(ArgMap &map, std::set<std::string> const &parsed_flags) const {
  for (auto const &flag : parsed_flags)
    map.args[flag] = ArgValue{"1"};
}

void Program::assign_options(ArgMap &map, std::map<std::string, std::string> const &parsed_options) const {
  for (auto const &option : parsed_options)
    map.args[option.first] = ArgValue{option.second};
}

ArgMap Program::operator()(int argc, char const *argv[]) const {
  std::span<char const *> args{argv, static_cast<std::size_t>(argc)};
  parsing::ArgumentParser parser(*this, args);
  return assign_args(parser());
}

ArgMap Program::assign_args(ParseResult const &parse_result) const {
  ArgMap map;
  assign_args_into(map, parse_result);
  return map;
}

void Program::assign_args_into(ArgMap &map, ParseResult const &parse_result) const {
  map.cmd_name = parse_result.cmd_name;
  if (parse_result.subcmd != nullptr) {
    auto const &subcmd = cmds.at(parse_result.subcmd->cmd_name);
    map.subcmd = memory::ValuePtr(std::make_unique<ArgMap>());
    subcmd->assign_args_into(*map.subcmd, *parse_result.subcmd);
  }
  assign_positional_args(map, parse_result.positional);
  assign_flags(map, parse_result.flags);
  assign_options(map, parse_result.options);
}

} // namespace opzioni
