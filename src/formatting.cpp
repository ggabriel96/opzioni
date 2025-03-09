#include "formatting.hpp"

namespace opz {

// +--------------------------------------+
// |             CmdHelpEntry             |
// +--------------------------------------+

[[nodiscard]] std::string CmdHelpEntry::format_for_help_description() const noexcept {
  return std::string(introduction);
}

[[nodiscard]] std::string CmdHelpEntry::format_for_help_index() const noexcept {
  return this->format_for_usage_summary();
}

[[nodiscard]] std::string CmdHelpEntry::format_for_usage_summary() const noexcept { return std::string(name); }

// +--------------------------------------+
// |             ArgHelpEntry             |
// +--------------------------------------+

[[nodiscard]] bool ArgHelpEntry::has_abbrev() const noexcept { return !abbrev.empty(); }
[[nodiscard]] bool ArgHelpEntry::has_default() const noexcept { return default_value.has_value(); }
[[nodiscard]] bool ArgHelpEntry::has_implicit() const noexcept { return implicit_value.has_value(); }

[[nodiscard]] std::string ArgHelpEntry::format_base_usage() const noexcept {
  if (type == ArgType::POS) return std::string(name);

  auto const dashes = name.length() > 1 ? "--" : "-";
  if (type == ArgType::OPT) {
    auto val = fmt::format("<{}>", has_abbrev() ? abbrev : name);
    if (implicit_value) val = "[" + val + "]";
    return fmt::format("{}{} {}", dashes, name, val);
  }

  return fmt::format("{}{}", dashes, name);
}

[[nodiscard]] std::string ArgHelpEntry::format_for_help_description() const noexcept {
  return fmt::format(
    fmt::runtime(help),
    fmt::arg("name", name),
    fmt::arg("abbrev", abbrev),
    fmt::arg("cmd_name", cmd_name),
    fmt::arg("default_value", default_value.value_or("")),
    fmt::arg("implicit_value", implicit_value.value_or("")));
  // fmt::arg("gather_amount", gather_amount));
}

[[nodiscard]] std::string ArgHelpEntry::format_for_help_index() const noexcept {
  if (type == ArgType::POS) return format_base_usage();
  if (has_abbrev()) return fmt::format("-{}, {}", abbrev, format_base_usage());
  if (name.length() == 1) return format_base_usage();
  return fmt::format("    {}", format_base_usage());
}

[[nodiscard]] std::string ArgHelpEntry::format_for_usage_summary() const noexcept {
  auto format = format_base_usage();
  if (type == ArgType::POS) format = "<" + format + ">";
  if (!is_required) format = "[" + format + "]";
  return format;
}

bool ArgHelpEntry::operator<(ArgHelpEntry const &other) const noexcept {
  bool const lhs_is_positional = this->type == ArgType::POS;
  bool const rhs_is_positional = other.type == ArgType::POS;

  if (lhs_is_positional && rhs_is_positional) return false; // don't move positionals relative to each other
  if (!lhs_is_positional && !rhs_is_positional) return this->name < other.name; // sort non-positionals by name
  return lhs_is_positional;                                                     // sort positionals before other types
}

// +---------------------------------------+
// |             HelpFormatter             |
// +---------------------------------------+

void HelpFormatter::print_title() const noexcept { fmt::print("{} {}\n", name, version); }

void HelpFormatter::print_intro() const noexcept {
  if (introduction.length() <= msg_width) {
    fmt::print("{}\n", introduction);
  } else {
    fmt::print("{}\n", limit_string_within(introduction, msg_width));
  }
}

void HelpFormatter::print_long_usage() const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::transform;
  using std::views::drop, std::views::filter, std::views::take;

  std::vector<std::string> words;
  words.reserve(1 + args.size() + cmds.size());

  auto insert = std::back_inserter(words);
  words.emplace_back(name);
  transform(args | filter(&ArgHelpEntry::is_required), insert, &ArgHelpEntry::format_for_usage_summary);
  transform(args | filter(&ArgHelpEntry::has_default), insert, &ArgHelpEntry::format_for_usage_summary);

  if (cmds.size() == 1) {
    words.push_back(format("{{{}}}", cmds.front().format_for_usage_summary()));
  } else if (cmds.size() > 1) {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", cmds.front().format_for_usage_summary()));
    transform(cmds | drop(1) | take(cmds.size() - 2), insert, &CmdHelpEntry::format_for_usage_summary);
    words.push_back(format("{}}}", cmds.back().format_for_usage_summary()));
  }

  // -4 because we'll later print a left margin of 4 spaces
  auto const split_lines = limit_within(std::span(words), msg_width - 4);
  fmt::print("Usage:\n");
  for (auto const &line : split_lines) {
    fmt::print("    {}\n", join(line, " "));
  }
}

void HelpFormatter::print_help() const noexcept {
  std::string_view pending_nl;
  // using same padding size for all arguments so they stay aligned
  auto const padding_size = help_padding_size();

  if (amount_pos > 0) {
    fmt::print("Positionals:\n");
    for (auto const &arg : args | std::views::take(amount_pos)) {
      print_arg_help(arg, padding_size);
    }
    pending_nl = "\n";
  }

  if (args.size() > amount_pos) {
    fmt::print("{}Options & Flags:\n", pending_nl);
    for (auto const &arg : args | std::views::drop(amount_pos)) {
      print_arg_help(arg, padding_size);
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!cmds.empty()) {
    fmt::print("{}Commands:\n", pending_nl);
    for (auto const &arg : cmds) {
      print_arg_help(arg, padding_size);
    }
  }
}

void HelpFormatter::print_details() const noexcept {
  // if (details.empty())
  //   return;
  // if (details.length() <= msg_width)
  //   out << details << nl;
  // else
  //   out << limit_string_within(details, msg_width) << nl;
}

[[nodiscard]] std::size_t HelpFormatter::help_padding_size() const noexcept {
  using std::views::transform;
  auto const required_length = [](auto const &arg) -> std::size_t { return arg.format_for_help_index().length(); };
  std::size_t const required_length_args = args.empty() ? 0 : std::ranges::max(args | transform(required_length));
  std::size_t const required_length_cmds = cmds.empty() ? 0 : std::ranges::max(cmds | transform(required_length));
  return std::max(required_length_args, required_length_cmds);
  return 0;
}

} // namespace opz
