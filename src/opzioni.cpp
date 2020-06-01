#include <algorithm>

#include "opzioni.hpp"

namespace opz {

template <>
Arg<bool> &Arg<bool>::as_flag(bool flag_value) {
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
    // but is longer than two characters and has no equals, hence short option
    // with value (e.g. `-O2`) (possibility of many short flags has already been
    // tested for)
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

bool should_stop_parsing(std::string const &whole_arg) {
  auto const all_dashes = whole_arg.find_first_not_of('-') == std::string::npos;
  return whole_arg.length() == 2 && all_dashes;
}

bool is_positional(std::string const &whole_arg) {
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  return idx_first_not_dash == 0;
}

bool ArgParser::is_multiple_short_flags(std::string const &whole_arg) const {
  auto const flags = whole_arg.substr(1);
  auto const idx_first_not_dash = whole_arg.find_first_not_of('-');
  auto is_option = [options = this->options](char c) {
    auto const opt = options.find(std::string(1, c));
    return opt != options.end() && opt->second.flag_value.has_value();
  };
  return idx_first_not_dash == 1 && flags.length() > 1 &&
         std::all_of(flags.begin(), flags.end(), is_option);
}

ArgMap ArgParser::parse_args(int argc, char const *argv[]) const {
  ArgMap arg_map;
  int positional_idx = 0;
  for (int i = 1; i < argc; ++i) {
    auto const whole_arg = std::string(argv[i]);
    if (should_stop_parsing(whole_arg)) {
      arg_map.remaining_args = get_remaining_args(i + 1, argc, argv);
      break;
    }
    if (is_positional(whole_arg)) {
      auto const positional_arg = this->positional_args.at(positional_idx);
      auto const parsed_value = positional_arg.converter(whole_arg);
      arg_map.args[positional_arg.name] = ArgValue{parsed_value};
      ++positional_idx;
    } else if (is_multiple_short_flags(whole_arg)) {
      // support for arguments like -abc where a, b and c are flags
      auto const flags = whole_arg.substr(1);
      std::for_each(
          flags.begin(), flags.end(), [this, &arg_map](char const &c) {
            auto const flag_arg = this->options.at(std::string(1, c));
            arg_map.args[flag_arg.name] = ArgValue{flag_arg.flag_value};
          });
    } else {
      auto const split = split_arg(whole_arg);
      if (!this->options.contains(split.name))
        throw UnknownArgument(fmt::format("Unknown argument `{}`", split.name));
      auto const arg = this->options.at(split.name);
      if (arg.is_flag()) {
        if (split.value.has_value())
          throw FlagHasValue(
              fmt::format("Argument `{0}` is a flag, thus cannot take a value "
                          "as in `{1}`. Simply set it with `-{0}`",
                          arg.name, whole_arg));
        arg_map.args[split.name] = ArgValue{arg.flag_value};
      } else {
        auto const arg_value = [&]() {
          if (split.value) {
            // if `split_arg` managed to parse a value, use it
            return *split.value;
          }
          if (i + 1 < argc) {
            // if we have not yet exhausted argv, interpret next element as
            // value
            ++i;
            return std::string(argv[i]);
          } else {
            throw MissingValue(
                fmt::format("Missing value for option `{}`", whole_arg));
          }
        }();
        auto const parsed_value = arg.converter(arg_value);
        arg_map.args[split.name] = ArgValue{parsed_value};
      }
    }
  }
  arg_map.set_defaults_for_missing_options(this->options);
  return arg_map;
}

void ArgMap::set_defaults_for_missing_options(
    std::unordered_map<std::string, ArgInfo> const &options) {
  std::for_each(options.begin(), options.end(), [this](auto const &item) {
    auto const &option = item.second;
    if (!this->args.contains(option.name) && option.default_value.has_value()) {
      this->args[option.name] = ArgValue{option.default_value};
    }
  });
}

} // namespace opz
