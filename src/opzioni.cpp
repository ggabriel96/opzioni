#include "opzioni.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <memory>
#include <ranges>

namespace opzioni {

template <> Arg<bool> &Arg<bool>::as_flag(bool flag_value) {
  this->flag_value = flag_value;
  this->default_value = !flag_value;
  return *this;
}

SplitArg split_arg(std::string const &whole_arg) {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == std::string::npos) {
    // all dashes?
    return SplitArg{.num_of_dashes = whole_arg.length(), .name = "", .value = std::nullopt};
  }
  auto const eq_idx = whole_arg.find('=', num_of_dashes);
  if (num_of_dashes == 1 && whole_arg.length() > 2 && eq_idx == std::string::npos) {
    // has one dash, hence flag
    // but is longer than 2 characters
    // has no equals
    // hence short option with value (e.g. `-O2`)
    // (possibility of many short flags has already been tested for)
    auto const name = whole_arg.substr(1, 1);
    auto const value = whole_arg.substr(2);
    return SplitArg{.num_of_dashes = num_of_dashes, .name = name, .value = value};
  }
  if (eq_idx == std::string::npos) {
    // has dashes prefix, but no equals,
    // hence either flag or option with next CLI argument as value
    auto const name = whole_arg.substr(num_of_dashes);
    return SplitArg{.num_of_dashes = num_of_dashes, .name = name, .value = std::nullopt};
  }
  // has dashes prefix and equals, hence option with value
  auto const name = whole_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
  auto const value = whole_arg.substr(eq_idx + 1);
  return SplitArg{.num_of_dashes = num_of_dashes, .name = name, .value = value};
}

bool is_two_dashes(std::string const &whole_arg) {
  auto const all_dashes = whole_arg.find_first_not_of('-') == std::string::npos;
  return whole_arg.length() == 2 && all_dashes;
}

bool is_positional(std::string const &whole_arg) {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 0;
}

bool Program::is_flag(std::string const &whole_arg) const noexcept {
  auto const flag = whole_arg.substr(2);
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 2 && flag.length() >= 1 && this->options.contains(flag);
}

bool Program::is_multiple_short_flags(std::string const &whole_arg) const noexcept {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  auto const flags = whole_arg.substr(1);
  auto const all_short_flags = std::all_of(flags.begin(), flags.end(),
                                           [this](char const &c) { return this->options.contains(std::string(1, c)); });
  return idx_first_not_dash == 1 && flags.length() >= 2 && all_short_flags;
}

void Program::assign_positional_args(ArgMap *map, std::vector<std::string> const &parsed_positional) const {
  if (parsed_positional.size() < positional_args.size()) {
    auto const names = std::ranges::transform_view(positional_args, [](auto const &arg) { return arg.name; });
    throw ArgumentNotFound(fmt::format("Missing {} positional argument{:s<{}}: `{}`", names.size(), "",
                                       static_cast<int>(names.size() != 1), fmt::join(names, "`, `")));
  }
  if (parsed_positional.size() > positional_args.size()) {
    auto const args = std::ranges::drop_view(parsed_positional, positional_args.size());
    throw UnknownArgument(fmt::format("Got {} unexpected positional argument{:s<{}}: `{}`", args.size(), "",
                                      static_cast<int>(args.size() != 1), fmt::join(args, "`, `")));
  }
  for (size_t i = 0; i < positional_args.size(); ++i) {
    auto const &arg = positional_args[i];
    map->args[arg.name] = ArgValue{arg.converter(parsed_positional[i])};
  }
}

void Program::assign_flags(ArgMap *map, std::set<std::string> const &parsed_flags) const {
  auto spec_flags = std::ranges::transform_view(options, [](auto const &item) { return item.second; }) |
                    std::views::filter([](auto const &option) { return option.flag_value.has_value(); });
  for (auto const &flag : spec_flags) {
    if (parsed_flags.contains(flag.name))
      map->args[flag.name] = ArgValue{flag.flag_value};
    else
      map->args[flag.name] = ArgValue{flag.default_value};
  }
}

void Program::assign_options(ArgMap *map, std::map<std::string, std::string> const &parsed_options) const {
  auto parsed_flags_with_value = std::ranges::filter_view(parsed_options,
                                                          [&](auto const &item) {
                                                            auto const &option = options.find(item.first);
                                                            return option != options.end() && option->second.is_flag();
                                                          }) |
                                 std::views::transform([](auto const &item) { return item.first; });
  if (!std::ranges::empty(parsed_flags_with_value))
    throw FlagHasValue(fmt::format("Arguments `{}` are flags, thus cannot "
                                   "take values. Simply set them with `-{}`",
                                   fmt::join(parsed_flags_with_value.begin(), parsed_flags_with_value.end(), "`, `"),
                                   fmt::join(parsed_flags_with_value.begin(), parsed_flags_with_value.end(), " -")));
  auto spec_options = std::ranges::filter_view(options, [](auto const item) { return !item.second.is_flag(); }) |
                      std::views::transform([](auto const &item) { return item.second; });
  for (auto const &option : spec_options) {
    if (auto const parsed_value = parsed_options.find(option.name); parsed_value != parsed_options.end())
      map->args[option.name] = ArgValue{option.converter(parsed_value->second)};
    else
      map->args[option.name] = ArgValue{option.default_value};
  }
}

[[no_discard]] decltype(auto) Program::find_cmd(std::string_view const arg) const noexcept {
  return std::find_if(cmds.begin(), cmds.end(), [&arg](auto const &ptr) { return ptr->name == arg; });
}

ArgMap Program::parse(int argc, char const *argv[]) const { return convert_args(parse_args(argc, argv)); }

ArgMap Program::convert_args(ParseResult &&parse_result) const {
  ArgMap map;
  convert_args_into(&map, &parse_result);
  return map;
}

ValuePtr<ArgMap> Program::convert_args(ParseResult *parse_result) const {
  auto map = ValuePtr(std::make_unique<ArgMap>());
  convert_args_into(map.get(), parse_result);
  return map;
}

void Program::convert_args_into(ArgMap *map, ParseResult *parse_result) const {
  map->cmd_name = parse_result->cmd_name;
  if (parse_result->subcmd != nullptr) {
    auto const subcmd = find_cmd(parse_result->subcmd->cmd_name);
    map->subcmd = (*subcmd)->convert_args(parse_result->subcmd.get());
  }
  assign_positional_args(map, parse_result->positional);
  assign_flags(map, parse_result->flags);
  assign_options(map, parse_result->options);
}

// +------------+
// | parse_args |
// +------------+

ParseResult Program::parse_args(int argc, char const *argv[]) const {
  ParseResult parse_result;
  parse_args_into(&parse_result, argc, argv);
  return parse_result;
}

ValuePtr<ParseResult> Program::parse_args(int argc, char const *argv[], int start) const {
  auto parse_result = ValuePtr(std::make_unique<ParseResult>());
  parse_args_into(parse_result.get(), argc, argv, start);
  return parse_result;
}

void Program::parse_args_into(ParseResult *parse_result, int argc, char const *argv[], int start) const {
  parse_result->cmd_name = std::string(argv[start]);
  for (int i = start + 1; i < argc; ++i) {
    auto const whole_arg = std::string(argv[i]);
    if (auto const subcmd = find_cmd(whole_arg); subcmd != cmds.end()) {
      parse_result->subcmd = (*subcmd)->parse_args(argc, argv, i);
      break;
    }
    if (is_two_dashes(whole_arg)) {
      int const start_idx = i + 1;
      int const remaining_args_count = argc - start_idx;
      parse_result->positional.reserve(parse_result->positional.size() + remaining_args_count);
      std::copy_n(argv + start_idx, remaining_args_count, std::back_inserter(parse_result->positional));
      break;
    }
    if (is_positional(whole_arg)) {
      parse_result->positional.push_back(whole_arg);
    } else if (is_multiple_short_flags(whole_arg)) {
      auto const flags = whole_arg.substr(1);
      for (char const &flag : flags) {
        parse_result->flags.insert(std::string(1, flag));
      }
    } else if (is_flag(whole_arg)) {
      parse_result->flags.insert(whole_arg.substr(2));
    } else {
      auto const split = split_arg(whole_arg);
      if (split.value) {
        parse_result->options[split.name] = *split.value;
      } else if (i + 1 < argc) {
        // if we have not yet exhausted argv,
        // interpret next element as value
        ++i;
        parse_result->options[split.name] = std::string(argv[i]);
      } else {
        throw ParseError(fmt::format("Could not parse option `{}`. Perhaps you forgot to provide a value?", whole_arg));
      }
    }
  }
}

} // namespace opzioni
