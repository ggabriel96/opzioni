#include <algorithm>
#include <cctype>
#include <ranges>

#include "opzioni.hpp"
#include <fmt/format.h>

namespace opz {

template <> Arg<bool> &Arg<bool>::as_flag(bool flag_value) {
  this->flag_value = flag_value;
  this->default_value = !flag_value;
  return *this;
}

SplitArg split_arg(std::string const &whole_arg) {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == std::string::npos) {
    // all dashes?
    return SplitArg{
        .num_of_dashes = whole_arg.length(), .name = "", .value = std::nullopt};
  }
  auto const eq_idx = whole_arg.find('=', num_of_dashes);
  if (num_of_dashes == 1 && whole_arg.length() > 2 &&
      eq_idx == std::string::npos) {
    // has one dash, hence flag
    // but is longer than 2 characters
    // has no equals
    // hence short option with value (e.g. `-O2`)
    // (possibility of many short flags has already been tested for)
    auto const name = whole_arg.substr(1, 1);
    auto const value = whole_arg.substr(2);
    return SplitArg{
        .num_of_dashes = num_of_dashes, .name = name, .value = value};
  }
  if (eq_idx == std::string::npos) {
    // has dashes prefix, but no equals,
    // hence either flag or option with next CLI argument as value
    auto const name = whole_arg.substr(num_of_dashes);
    return SplitArg{
        .num_of_dashes = num_of_dashes, .name = name, .value = std::nullopt};
  }
  // has dashes prefix and equals, hence option with value
  auto const name = whole_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
  auto const value = whole_arg.substr(eq_idx + 1);
  return SplitArg{.num_of_dashes = num_of_dashes, .name = name, .value = value};
}

auto get_remaining_args(int start_idx, int argc, char const *argv[]) {
  auto remaining_args = std::make_unique<std::vector<std::string>>();
  int const remaining_args_count = argc - start_idx;
  remaining_args->reserve(remaining_args_count);
  std::copy_n(argv + start_idx, remaining_args_count,
              std::back_inserter(*remaining_args));
  return remaining_args;
}

bool all_chars(std::string const &s) {
  return std::all_of(s.begin(), s.end(),
                     [](char const &c) { return std::isalpha(c) != 0; });
}

bool should_stop_parsing(std::string const &whole_arg) {
  auto const all_dashes = whole_arg.find_first_not_of('-') == std::string::npos;
  return whole_arg.length() == 2 && all_dashes;
}

bool is_positional(std::string const &whole_arg) {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 0;
}

bool is_multiple_short_flags(std::string const &whole_arg) {
  auto const flags = whole_arg.substr(1);
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 1 && flags.length() > 1 && all_chars(flags);
}

bool is_flag(std::string const &whole_arg) {
  auto const flag = whole_arg.substr(2);
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 2 && flag.length() > 1 && all_chars(flag);
}

ArgMap ArgParser::parse(int argc, char const *argv[]) const {
  return convert_args(parse_args(argc, argv));
}

void ArgParser::assign_positional_args(
    ArgMap &map, std::vector<std::string> const &positional_args) const {
  if (positional_args.size() < this->positional_args.size()) {
    auto const names = std::ranges::transform_view(
        this->positional_args, [](auto const &arg) { return arg.name; });
    throw ArgumentNotFound(fmt::format("Missing positional arguments: `{}`",
                                       fmt::join(names, "`, `")));
  }
  for (size_t i = 0; i < this->positional_args.size(); ++i) {
    auto const &arg = this->positional_args[i];
    map.args[arg.name] = ArgValue{arg.converter(positional_args[i])};
  }
}

void ArgParser::assign_flags(
    ArgMap &map, std::unordered_set<std::string> const &flags) const {
  for (auto const &flag : flags) {
    auto const arg = options.at(flag);
    map.args[arg.name] = ArgValue{arg.flag_value};
  }
}

void ArgParser::assign_options(
    ArgMap &map,
    std::unordered_map<std::string, std::string> const &options) const {
  for (auto const &[name, value] : options) {
    auto const arg = this->options.at(name);
    if (arg.is_flag()) {
      throw FlagHasValue(fmt::format("Argument `{0}` is a flag, thus cannot "
                                     "take a value. Simply set it with `-{0}`",
                                     name));
    }
    map.args[name] = ArgValue{arg.converter(value)};
  }
}

ArgMap ArgParser::convert_args(ParseResult &&parse_result) const {
  ArgMap map;
  map.remaining_args = std::move(parse_result.remaining);
  assign_positional_args(map, parse_result.positional);
  assign_flags(map, parse_result.flags);
  assign_options(map, parse_result.options);
  return map;
}

ParseResult ArgParser::parse_args(int argc, char const *argv[]) const {
  ParseResult parse_result;
  for (int i = 1; i < argc; ++i) {
    auto const whole_arg = std::string(argv[i]);
    if (should_stop_parsing(whole_arg)) {
      parse_result.remaining = get_remaining_args(i + 1, argc, argv);
      break;
    }
    if (is_positional(whole_arg)) {
      parse_result.positional.push_back(whole_arg);
    } else if (is_multiple_short_flags(whole_arg)) {
      auto const flags = whole_arg.substr(1);
      for (char const &flag : flags) {
        parse_result.flags.insert(std::string(1, flag));
      }
    } else if (is_flag(whole_arg)) {
      parse_result.flags.insert(whole_arg.substr(2));
    } else {
      auto const split = split_arg(whole_arg);
      if (split.value) {
        parse_result.options[split.name] = *split.value;
      } else if (i + 1 < argc) {
        // if we have not yet exhausted argv,
        // interpret next element as value
        ++i;
        parse_result.options[split.name] = std::string(argv[i]);
      } else {
        throw ParseError(fmt::format("Could not parse option `{}`. Perhaps you "
                                     "forgot to provide a value?",
                                     whole_arg));
      }
    }
  }
  return parse_result;
}

} // namespace opz
