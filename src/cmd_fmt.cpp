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
[[nodiscard]] bool ArgHelpEntry::has_group() const noexcept { return grp_kind != GroupKind::NONE; }
[[nodiscard]] bool ArgHelpEntry::not_has_group() const noexcept { return !this->has_group(); }

[[nodiscard]] std::string ArgHelpEntry::format_base_usage() const noexcept {
  if (kind == ArgKind::POS) return std::string(name);

  if (kind == ArgKind::OPT) {
    auto val = fmt::format("<{}>", has_abbrev() ? abbrev : name);
    if (implicit_value) val = "[" + val + "]";
    // TODO: can we do something other than putting the equal sign here
    // to differentiate option values and positionals?
    return fmt::format("--{}={}", name, val);
  }

  return fmt::format("--{}", name);
}

[[nodiscard]] std::string ArgHelpEntry::format_for_usage() const noexcept {
  auto format = format_base_usage();
  if (kind == ArgKind::POS) format = "<" + format + ">";
  if (!is_required && !has_group()) format = "[" + format + "]";
  return format;
}

[[nodiscard]] std::string ArgHelpEntry::format_for_index_entry() const noexcept {
  if (kind == ArgKind::POS) return format_base_usage();
  if (has_abbrev()) return fmt::format("-{}, {}", abbrev, format_base_usage());
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
    fmt::print(f, "{}\n", limit_line_within(introduction, msg_width).to_str_lines());
  }
}

std::string format_group(
  std::vector<ArgHelpEntry>::const_iterator &it, std::vector<ArgHelpEntry>::const_iterator const container_end
) noexcept {
  bool const required_group = it->is_required;
  std::string line(1, required_group ? '(' : '[');
  auto const separator = it->grp_kind == GroupKind::MUTUALLY_EXCLUSIVE ? " | " : " ";
  auto const end_it =
    std::find_if(std::next(it), container_end, [it](auto const &entry) { return entry.grp_id != it->grp_id; });
  for (; it < end_it; std::advance(it, 1)) {
    line += it->format_for_usage();
    if (std::next(it) != end_it) line += separator;
  }
  line.push_back(required_group ? ')' : ']');
  return line;
}

void CmdFmt::print_usage(std::FILE *f) const noexcept {
  using fmt::format, fmt::join;
  using std::ranges::transform;
  using std::views::drop, std::views::take;

  // using a vector of words to represent indivisible substrings,
  // otherwise we could add a newline in unwanted places like [--config\n<config>]
  std::vector<std::string> words;
  words.reserve(1 + args.size() + subcmds.size());
  for (auto const name : parent_cmds_names) {
    words.emplace_back(name);
  }
  words.emplace_back(name);

  for (auto it = args.begin(); it < args.end();) {
    if (it->has_group()) {
      words.push_back(format_group(it, args.end()));
    } else {
      words.push_back(it->format_for_usage());
      std::advance(it, 1);
    }
  }

  if (subcmds.size() == 1) {
    words.push_back(format("{{{}}}", subcmds.front().format_for_index_entry()));
  } else if (subcmds.size() > 1) {
    // don't need space after commas because we'll join words with spaces afterwards
    words.push_back(format("{{{},", subcmds.front().format_for_index_entry()));
    transform(
      subcmds | drop(1) | take(subcmds.size() - 2), std::back_inserter(words), &CmdHelpEntry::format_for_index_entry
    );
    words.push_back(format("{}}}", subcmds.back().format_for_index_entry()));
  }

  // -2 because we'll print a left margin of 2 spaces in the for-loop below
  auto const paragraph = limit_within(std::span(words), msg_width - 2);
  fmt::print(f, "USAGE:\n");
  for (auto const &line : paragraph.lines()) {
    fmt::print(f, "  {}\n", join(line.words(), " "));
  }
}

void CmdFmt::print_help(std::FILE *f) const noexcept {
  using std::ranges::to;
  using std::views::transform;

  // using same padding size for all arguments so they stay aligned
  auto const arg_index_entries =
    args | transform(&ArgHelpEntry::format_for_index_entry) | to<std::vector<std::string>>();
  auto const subcmd_index_entries =
    subcmds | transform(&CmdHelpEntry::format_for_index_entry) | to<std::vector<std::string>>();
  std::size_t const required_length_args =
    arg_index_entries.empty() ? 0 : std::ranges::max(arg_index_entries | transform(&std::string::length));
  std::size_t const required_length_cmds =
    subcmd_index_entries.empty() ? 0 : std::ranges::max(subcmd_index_entries | transform(&std::string::length));
  auto const padding_size = std::max(required_length_args, required_length_cmds);

  std::string_view pending_nl;
  if (!args.empty()) {
    fmt::print(f, "ARGUMENTS:\n");
    int i = 0;
    for (auto const &arg : args) {
      print_arg_help(f, arg_index_entries[i], arg.format_for_index_description(), padding_size);
      i += 1;
    }
    pending_nl = "\n";
  } else {
    pending_nl = "";
  }

  if (!subcmds.empty()) {
    fmt::print(f, "{}SUBCOMMANDS:\n", pending_nl);
    int i = 0;
    for (auto const &subcmd : subcmds) {
      print_arg_help(f, subcmd_index_entries[i], subcmd.format_for_index_description(), padding_size);
      i += 1;
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

void CmdFmt::print_arg_help(
  std::FILE *f, std::string const &index_entry, std::string const &index_description, std::size_t const padding_size
) const noexcept {
  // -8 because we print 2 spaces of left margin and 2 spaces of indentation for descriptions longer than 1 line
  // then add 4 spaces between the arg usage and description
  auto const paragraph = limit_line_within(index_description, msg_width - padding_size - 8);

  fmt::print(f, "  {:<{}}    {}\n", index_entry, padding_size, fmt::join(paragraph.lines().front().words(), " "));

  for (auto const &line : paragraph.lines() | std::views::drop(1)) {
    // the same 2 spaces of left margin, then additional 4 spaces of indentation
    fmt::print(f, "  {: >{}}      {}\n", ' ', padding_size, fmt::join(line.words(), " "));
  }
}

} // namespace opz
