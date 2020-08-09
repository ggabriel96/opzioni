#include "parsing.hpp"

namespace opzioni {

namespace parsing {

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

ParseResult ArgumentParser::operator()() {
  result.cmd_name = std::string(args[0]);
  for (std::size_t index = 1; index < args.size();) {
    index += std::visit(*this, decide_type(index));
  }
  return result;
}

alternatives ArgumentParser::decide_type(std::size_t index) const noexcept {
  auto const arg = std::string(args[index]);

  if (arg.length() == 2 && arg[0] == '-' && arg[1] == '-')
    return parsing::DashDash{index};

  if (auto const cmd_name = spec.is_subcmd(arg); cmd_name)
    return parsing::Subcommand{*cmd_name, index};

  if (auto const positional = spec.is_positional(arg); positional)
    return parsing::Positional{*positional};

  if (auto const flags = spec.is_short_flags(arg); flags)
    return parsing::ManyFlags{*flags};

  if (auto const flag = spec.is_long_flag(arg); flag)
    return Flag{*flag};

  if (auto const option = spec.is_option(arg); option)
    return Option{*option, index};

  return Unknown{arg};
}

std::size_t ArgumentParser::operator()(DashDash dd) {
  std::size_t const next_args_count = args.size() - (dd.index + 1);
  result.positional.reserve(result.positional.size() + next_args_count);
  std::copy_n(args.last(next_args_count).begin(), next_args_count, std::back_inserter(result.positional));
  return 1 + next_args_count; // +1 because we also count the dash-dash
}

std::size_t ArgumentParser::operator()(Flag flag) {
  result.flags.insert(flag.name);
  return 1;
}

std::size_t ArgumentParser::operator()(ManyFlags flags) {
  for (char const &flag : flags.chars) {
    result.flags.insert(std::string(1, flag));
  }
  return 1;
}

std::size_t ArgumentParser::operator()(Option option) {
  if (option.arg.value) {
    result.options[option.arg.name] = *option.arg.value;
    return 1;
  } else if (option.index + 1 < args.size()) {
    // if we have not yet exhausted args,
    // interpret next element as value
    auto const value = std::string(args[option.index + 1]);
    result.options[option.arg.name] = value;
    return 2;
  } else {
    throw ParseError(fmt::format("Could not parse argument `{}`. Perhaps you forgot to provide a value?", option.arg.name));
  }
}

std::size_t ArgumentParser::operator()(Positional positional) {
  result.positional.push_back(positional.value);
  return 1;
}

std::size_t ArgumentParser::operator()(Subcommand subcmd) {
  auto const remaining_args_count = args.size() - subcmd.index;
  auto const &subprogram = spec.get_cmd(subcmd.name);
  auto subargs = args.last(remaining_args_count);
  ArgumentParser subparser{subprogram, subargs};
  result.subcmd = memory::ValuePtr(std::make_unique<ParseResult>(std::move(subparser())));
  return remaining_args_count;
}

std::size_t ArgumentParser::operator()(Unknown unknown) {
  throw UnknownArgument(unknown.arg);
}

} // namespace parsing
} // namespace opzioni
