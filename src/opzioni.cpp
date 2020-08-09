#include "opzioni.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <memory>
#include <ranges>

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

ParsedOption parse_option(std::string const &cli_arg) {
  auto const num_of_dashes = cli_arg.find_first_not_of('-');
  auto const eq_idx = cli_arg.find('=', num_of_dashes);
  bool const has_equals = eq_idx != std::string::npos;
  if (has_equals) {
    // long or short option with value
    auto const name = cli_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
    auto const value = cli_arg.substr(eq_idx + 1);
    return {.num_of_dashes = num_of_dashes, .name = name, .value = value};
  } else if (num_of_dashes == 1 && cli_arg.length() > 2) {
    // has one dash, hence short option
    // but is longer than 2 characters and has no equals
    // hence short option with value (e.g. `-O2`)
    // (possibility of many short flags has already been tested for)
    auto const name = cli_arg.substr(1, 1);
    auto const value = cli_arg.substr(2);
    return {.num_of_dashes = num_of_dashes, .name = name, .value = value};
  } else {
    // no equals and has 1 or 2 prefix dashes
    // hence option with next CLI argument as value
    auto const name = cli_arg.substr(num_of_dashes);
    return {.num_of_dashes = num_of_dashes, .name = name, .value = std::nullopt};
  }
}

auto count_dashes(std::string const &cli_arg) { return cli_arg.find_first_not_of('-'); }

bool is_positional(std::string const &whole_arg) {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 0;
}

bool Program::is_flag(std::string const &name) const noexcept { return flags.contains(name); }

bool Program::arg_is_long_flag(std::string const &whole_arg) const noexcept {
  auto const name = whole_arg.substr(2);
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 2 && name.length() >= 2 && is_flag(name);
}

bool Program::arg_is_short_flags(std::string const &whole_arg) const noexcept {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  auto const all_short_flags =
      std::all_of(flags.begin(), flags.end(), [this](char const &c) { return this->is_flag(std::string(1, c)); });
  return idx_first_not_dash == 1 && flags.length() >= 1 && all_short_flags;
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

ArgMap Program::parse(int argc, char const *argv[]) const {
  std::span<char const *> args{argv, static_cast<std::size_t>(argc)};
  return assign_args(parse_args(args));
}

ParseResult Program::parse_args(std::span<char const *> args) const {
  ParseResult parse_result;
  parse_args_into(parse_result, args);
  return parse_result;
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
    map.subcmd = ValuePtr(std::make_unique<ArgMap>());
    subcmd->assign_args_into(*map.subcmd, *parse_result.subcmd);
  }
  assign_positional_args(map, parse_result.positional);
  assign_flags(map, parse_result.flags);
  assign_options(map, parse_result.options);
}

void Program::parse_args_into(ParseResult &parse_result, std::span<char const *> args) const {
  parse_result.cmd_name = std::string(args[0]);
  for (std::size_t i = 1; i < args.size(); ++i) {
    auto const whole_arg = std::string(args[i]);
    if (auto const num_of_dashes = count_dashes(whole_arg); num_of_dashes > 2) {
      throw TooManyDashes(fmt::format("Invalid argument `{}`. Did you mean `--`?", whole_arg));
    } else if (num_of_dashes == 2 && whole_arg.length() == 2) {
      std::size_t const remaining_args_count = args.size() - (i + 1);
      parse_result.positional.reserve(parse_result.positional.size() + remaining_args_count);
      std::copy_n(args.last(remaining_args_count).begin(), remaining_args_count,
                  std::back_inserter(parse_result.positional));
      break;
    }

    if (auto const subcmd = cmds.find(whole_arg); subcmd != cmds.end()) {
      parse_result.subcmd = ValuePtr(std::make_unique<ParseResult>());
      subcmd->second->parse_args_into(*parse_result.subcmd, args.last(args.size() - i));
      break;
    } else if (is_positional(whole_arg)) {
      parse_result.positional.push_back(whole_arg);
    } else if (arg_is_short_flags(whole_arg)) {
      auto const flags = whole_arg.substr(1);
      for (char const &flag : flags) {
        parse_result.flags.insert(std::string(1, flag));
      }
    } else if (arg_is_long_flag(whole_arg)) {
      parse_result.flags.insert(whole_arg.substr(2));
    } else { // only possibility left is an option
      auto const split = parse_option(whole_arg);
      if (auto const flag = this->flags.find(split.name); flag != this->flags.end()) {
        auto const message = fmt::format("Argument `{}` is a flag, thus cannot take a value. Simply set it with `--{}`",
                                         split.name, flag->second.name);
        throw FlagHasValue(message);
      }
      auto const option = options.find(split.name);
      if (option == options.end()) {
        throw UnknownArgument(fmt::format("Unknown option `{}` in `{}`", split.name, whole_arg));
      }
      if (split.value) {
        parse_result.options[option->second.name] = *split.value;
      } else if (i + 1 < args.size()) {
        // if we have not yet exhausted args,
        // interpret next element as value
        ++i;
        auto const value = std::string(args[i]);
        parse_result.options[option->second.name] = value;
      } else {
        throw ParseError(
            fmt::format("Could not parse argument `{}`. Perhaps you forgot to provide a value?", whole_arg));
      }
    }
  }
}

} // namespace opzioni
