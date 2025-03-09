#ifndef OPZIONI_FORMATTING_H
#define OPZIONI_FORMATTING_H

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "arg.hpp"
#include "command.hpp"
#include "strings.hpp"

namespace opz {

struct CmdHelpEntry {
  std::string_view name{};
  std::string_view introduction{};

  template <typename... Ts>
  explicit CmdHelpEntry(Command<Ts...> from) : name(from.name), introduction(from.introduction) {}

  [[nodiscard]] std::string format_for_help_description() const noexcept;
  [[nodiscard]] std::string format_for_help_index() const noexcept;
  [[nodiscard]] std::string format_for_usage_summary() const noexcept;

  auto operator<=>(CmdHelpEntry const &other) const noexcept { return name <=> other.name; }
};

struct ArgHelpEntry {
  ArgType type;
  std::string_view name;
  std::string_view cmd_name;
  std::string_view abbrev;
  std::string_view help;
  bool is_required;
  std::optional<std::string> default_value;
  std::optional<std::string> implicit_value;
  Action action;

  template <typename T>
  ArgHelpEntry(std::string_view cmd_name, Arg<T> from)
      : type(from.type),
        name(from.name),
        cmd_name(cmd_name),
        abbrev(from.abbrev),
        help(from.help),
        is_required(from.is_required),
        action(from.action) {
    if (from.default_value) default_value = fmt::format("{}", *from.default_value);
    if (from.implicit_value) implicit_value = fmt::format("{}", *from.implicit_value);
  }

  [[nodiscard]] bool has_abbrev() const noexcept;
  [[nodiscard]] bool has_default() const noexcept;
  [[nodiscard]] bool has_implicit() const noexcept;

  [[nodiscard]] std::string format_base_usage() const noexcept;
  [[nodiscard]] std::string format_for_help_description() const noexcept;
  [[nodiscard]] std::string format_for_help_index() const noexcept;
  [[nodiscard]] std::string format_for_usage_summary() const noexcept;

  bool operator<(ArgHelpEntry const &other) const noexcept;
};

struct HelpFormatter {
  std::string_view name;
  std::string_view version;
  std::string_view introduction;
  std::string_view parent_cmds_names{};
  std::vector<ArgHelpEntry> args;
  std::vector<CmdHelpEntry> cmds;
  std::size_t amount_pos;
  std::size_t msg_width;

  template <typename... CmdArgs> // don't really care about them here
  explicit HelpFormatter(Command<CmdArgs...> const &cmd, std::string_view parent_cmds_names)
      : name(cmd.name),
        version(cmd.version),
        introduction(cmd.introduction),
        parent_cmds_names(parent_cmds_names),
        amount_pos(cmd.amount_pos),
        msg_width(cmd.msg_width) {
    args.reserve(std::tuple_size_v<decltype(cmd.args)>);
    // clang-format off
    std::apply( // cast to void to suppress unused warning
      [this, &cmd](auto&&... arg) { (void) ((this->args.emplace_back(cmd.name, arg)), ...); },
      cmd.args
    );
    std::apply( // cast to void to suppress unused warning
      [this](auto&&... cmd) { (void) ((this->cmds.emplace_back(cmd)), ...); },
      cmd.subcmds
    );
    // clang-format on
    std::sort(args.begin(), args.end());
    std::sort(cmds.begin(), cmds.end());
  }

  void print_title() const noexcept;
  void print_intro() const noexcept;
  void print_usage() const noexcept;
  void print_help() const noexcept;
  void print_details() const noexcept;
  [[nodiscard]] std::size_t help_padding_size() const noexcept;

  void print_arg_help(auto const &arg, std::size_t const padding_size) const noexcept {
    using std::views::drop;
    auto const description = arg.format_for_help_description();
    // -8 because we print 4 spaces of left margin and 4 spaces of indentation for descriptions longer than 1 line
    // then -4 again because we add 4 spaces between the arg usage and description
    auto const description_lines = limit_within(description, msg_width - padding_size - 8 - 4);

    fmt::print(
      "    {:<{}}    {}\n", arg.format_for_help_index(), padding_size, fmt::join(description_lines.front(), " "));

    for (auto const &line : description_lines | drop(1)) {
      // the same 4 spaces of left margin, then additional 4 spaces of indentation
      fmt::print("    {: >{}}        {}\n", ' ', padding_size, fmt::join(line, " "));
    }
  }
};

} // namespace opz

#endif // OPZIONI_FORMATTING_H
