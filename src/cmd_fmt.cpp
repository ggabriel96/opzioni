#include "opzioni/cmd_fmt.hpp"

namespace opz {

// +--------------------------------------+
// |             CmdHelpEntry             |
// +--------------------------------------+

[[nodiscard]] std::string CmdHelpEntry::format_for_usage() const noexcept { return std::string(name); }

[[nodiscard]] std::string CmdHelpEntry::format_for_index_entry() const noexcept { return std::string(name); }

[[nodiscard]] std::string CmdHelpEntry::format_for_index_description() const noexcept {
  return std::string(introduction);
}

// +--------------------------------------+
// |             ArgHelpEntry             |
// +--------------------------------------+

[[nodiscard]] bool ArgHelpEntry::has_abbrev() const noexcept { return !abbrev.empty(); }
[[nodiscard]] bool ArgHelpEntry::has_default() const noexcept { return default_value.has_value(); }
[[nodiscard]] bool ArgHelpEntry::has_implicit() const noexcept { return implicit_value.has_value(); }

[[nodiscard]] std::string ArgHelpEntry::format_base_usage() const noexcept {
  if (kind == ArgKind::POS) return std::string(name);

  auto const dashes = name.length() > 1 ? "--" : "-";
  if (kind == ArgKind::OPT) {
    auto val = fmt::format("<{}>", has_abbrev() ? abbrev : name);
    if (implicit_value) val = "[" + val + "]";
    return fmt::format("{}{} {}", dashes, name, val);
  }

  return fmt::format("{}{}", dashes, name);
}

[[nodiscard]] std::string ArgHelpEntry::format_for_usage() const noexcept {
  auto format = format_base_usage();
  if (kind == ArgKind::POS) format = "<" + format + ">";
  if (!is_required) format = "[" + format + "]";
  return format;
}

[[nodiscard]] std::string ArgHelpEntry::format_for_index_entry() const noexcept {
  if (kind == ArgKind::POS) return format_base_usage();
  if (has_abbrev()) return fmt::format("-{}, {}", abbrev, format_base_usage());
  if (name.length() == 1) return format_base_usage();
  return fmt::format("    {}", format_base_usage());
}

[[nodiscard]] std::string ArgHelpEntry::format_for_index_description() const noexcept {
  return fmt::format(
    fmt::runtime(help),
    fmt::arg("name", name),
    fmt::arg("abbrev", abbrev),
    fmt::arg("cmd_name", cmd_name),
    fmt::arg("default_value", default_value.value_or("")),
    fmt::arg("implicit_value", implicit_value.value_or(""))
  );
  // fmt::arg("gather_amount", gather_amount));
}

bool ArgHelpEntry::operator<(ArgHelpEntry const &other) const noexcept {
  bool const lhs_is_positional = this->kind == ArgKind::POS;
  bool const rhs_is_positional = other.kind == ArgKind::POS;

  if (lhs_is_positional && rhs_is_positional) return false; // don't move positionals relative to each other
  if (!lhs_is_positional && !rhs_is_positional) return this->name < other.name; // sort non-positionals by name
  return lhs_is_positional;                                                     // sort positionals before other types
}

// +--------------------------------------+
// |                CmdFmt                |
// +--------------------------------------+

void CmdFmt::print_title(std::FILE *f) const noexcept {
  if (!parent_cmds_names.empty()) {
    fmt::print(f, "{} ", fmt::join(parent_cmds_names, " "));
  }
  fmt::print(f, "{}{: >{}}\n", name, version, version.size() + static_cast<int>(!version.empty()));
}

void CmdFmt::print_intro(std::FILE *f) const noexcept {
  if (introduction.length() <= msg_width) {
    fmt::print(f, "{}\n", introduction);
  } else {
    fmt::print(f, "{}\n", limit_string_within(introduction, msg_width));
  }
}

void CmdFmt::print_usage(std::FILE *f) const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::transform;
  using std::views::drop, std::views::filter, std::views::take;

  std::vector<std::string> words;
  words.reserve(1 + args.size() + subcmds.size());
  for (auto const name : parent_cmds_names) {
    words.emplace_back(name);
  }
  words.emplace_back(name);

  auto insert = std::back_inserter(words);
  transform(args | filter(&ArgHelpEntry::is_required), insert, &ArgHelpEntry::format_for_usage);
  transform(args | filter(&ArgHelpEntry::has_default), insert, &ArgHelpEntry::format_for_usage);

  if (subcmds.size() == 1) {
    words.push_back(format("{{{}}}", subcmds.front().format_for_index_entry()));
  } else if (subcmds.size() > 1) {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", subcmds.front().format_for_index_entry()));
    transform(subcmds | drop(1) | take(subcmds.size() - 2), insert, &CmdHelpEntry::format_for_index_entry);
    words.push_back(format("{}}}", subcmds.back().format_for_index_entry()));
  }

  // -4 because we'll later print a left margin of 4 spaces
  auto const split_lines = limit_within(std::span(words), msg_width - 4);
  fmt::print(f, "Usage:\n");
  for (auto const &line : split_lines) {
    fmt::print(f, "    {}\n", join(line, " "));
  }
}

void CmdFmt::print_help(std::FILE *f) const noexcept {
  std::string_view pending_nl;
  // using same padding size for all arguments so they stay aligned
  auto const padding_size = help_padding_size();

  if (!args.empty()) {
    fmt::print(f, "Arguments:\n");
    for (auto const &arg : args) {
      print_arg_help(arg, padding_size, f);
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!subcmds.empty()) {
    fmt::print(f, "{}Subcommands:\n", pending_nl);
    for (auto const &arg : subcmds) {
      print_arg_help(arg, padding_size, f);
    }
  }
}

void CmdFmt::print_details(std::FILE *f) const noexcept {
  // if (details.empty())
  //   return;
  // if (details.length() <= msg_width)
  //   out << details << nl;
  // else
  //   out << limit_string_within(details, msg_width) << nl;
}

[[nodiscard]] std::size_t CmdFmt::help_padding_size() const noexcept {
  using std::views::transform;
  auto const required_length = [](auto const &arg) -> std::size_t { return arg.format_for_index_entry().length(); };
  std::size_t const required_length_args = args.empty() ? 0 : std::ranges::max(args | transform(required_length));
  std::size_t const required_length_cmds = subcmds.empty() ? 0 : std::ranges::max(subcmds | transform(required_length));
  return std::max(required_length_args, required_length_cmds);
}

} // namespace opz
