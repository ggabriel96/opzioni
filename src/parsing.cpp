#include "parsing.hpp"

#include <optional>
#include <ranges>

namespace opzioni {

namespace parsing {

ArgMap ArgumentParser::operator()() {
  current_positional_idx = 0;
  map.cmd_name = std::string(args[0]);
  for (std::size_t index = 1; index < args.size();) {
    index += std::visit(*this, decide_type(index));
  }
  return map;
}

alternatives ArgumentParser::decide_type(std::size_t index) const noexcept {
  auto const arg = std::string(args[index]);

  if (arg.length() == 2 && arg[0] == '-' && arg[1] == '-')
    return parsing::DashDash{index};

  if (auto const cmd = spec.is_command(arg); cmd)
    return parsing::Command{*cmd, index};

  if (auto const positional = spec.is_positional(arg); positional)
    return parsing::Positional{index};

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
  for (std::size_t i = dd.index + 1; i < args.size();) {
    i += (*this)(Positional{i});
  }
  return 1 + next_args_count; // +1 because we also count the dash-dash
}

std::size_t ArgumentParser::operator()(Flag flag) {
  auto const arg = spec.flags.at(flag.name);
  arg.act(map, arg, std::nullopt);
  return 1;
}

std::size_t ArgumentParser::operator()(ManyFlags flags) {
  using std::views::transform;
  auto char_to_flag = [](char c) { return Flag{std::string(1, c)}; };
  for (auto const &flag : flags.chars | transform(char_to_flag)) {
    (*this)(flag);
  }
  return 1;
}

std::size_t ArgumentParser::operator()(Option option) {
  auto const arg = spec.options.at(option.arg.name);
  auto const gather_amount = arg.gather_n.amount == 0 ? args.size() - option.index : arg.gather_n.amount;
  if (option.arg.value) {
    if (gather_amount != 1) {
      throw MissingValue(fmt::format("Expected {} values for option `{}`, got 1", gather_amount, arg.name));
    }
    arg.act(map, arg, *option.arg.value);
    return 1;
  } else if (option.index + 1 + gather_amount <= args.size() &&
             std::holds_alternative<Positional>(decide_type(option.index + 1))) {
    // + 1 everywhere because `option.index` is the index in `args` that the option is.
    // From that index + 1 is where we start to parse values up to gather amount
    std::size_t count = 0;
    do {
      auto const value = std::string(args[option.index + 1 + count]);
      arg.act(map, arg, value);
      ++count;
    } while (count < gather_amount && std::holds_alternative<Positional>(decide_type(option.index + 1 + count)));
    return 1 + count;
  } else if (arg.set_value) {
    arg.act(map, arg, std::nullopt);
    return 1;
  } else {
    throw MissingValue(fmt::format("Expected {} values for option `{}`, got {}", gather_amount, arg.name,
                                   args.size() - (option.index + 1)));
  }
}

std::size_t ArgumentParser::operator()(Positional positional) {
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
    arg.act(map, arg, args[positional.index + count]);
  }
  ++current_positional_idx;
  return gather_amount;
}

std::size_t ArgumentParser::operator()(Command subcmd) {
  auto const remaining_args_count = args.size() - subcmd.index;
  auto subargs = args.last(remaining_args_count);
  ArgumentParser subparser{*subcmd.cmd->second, subargs};
  map.subcmd = memory::ValuePtr(std::make_unique<ArgMap>(std::move(subparser())));
  return remaining_args_count;
}

std::size_t ArgumentParser::operator()(Unknown unknown) { throw UnknownArgument(unknown.arg); }

} // namespace parsing
} // namespace opzioni
