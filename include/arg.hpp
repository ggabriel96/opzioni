#ifndef OPZIONI_ARG_H
#define OPZIONI_ARG_H

#include <optional>
#include <string_view>

#include "converters.hpp"
#include "fixed_string.hpp"
#include "strings.hpp"

namespace opz {

enum struct ArgType { POS, OPT, FLG };
std::string_view to_string(ArgType const at) noexcept;

enum struct Action {
  APPEND,
  ASSIGN,
  COUNT,
  CSV,
  PRINT_HELP,
  PRINT_VERSION,
  // TODO: custom user actions?
};

// +---------------------------------+
// |             ArgMeta             |
// +---------------------------------+

template <typename T>
struct ArgMeta {
  std::string_view help{};
  std::optional<bool> is_required{};
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};
  Action action = Action::ASSIGN;
};

constexpr static ArgMeta<bool> default_help = {
  .help = "Display this information",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
  .action = Action::PRINT_HELP,
};

constexpr static ArgMeta<bool> default_version = {
  .help = "Display {cmd_name}'s version",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
  .action = Action::PRINT_VERSION,
};

// +---------------------------------+
// |               Arg               |
// +---------------------------------+

template <typename T>
struct Arg {
  using value_type = T;

  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view help{};
  bool is_required = false;
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};
  Action action = Action::ASSIGN;
  //   std::size_t gather_amount = 1;
  // TODO: add deprecated option

  [[nodiscard]] constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  [[nodiscard]] constexpr bool has_default() const noexcept { return default_value.has_value(); }
  [[nodiscard]] constexpr bool has_implicit() const noexcept { return implicit_value.has_value(); }
};

// +---------------------------------+
// |       ArgMeta validations       |
// +---------------------------------+

template <FixedString Name, FixedString Abbrev, typename T>
constexpr void validate_common(ArgMeta<T> const &meta) {
  if (!is_valid_name(Name)) throw "Argument names must neither be empty nor contain any whitespace";
  if (Abbrev.size != 0 && (Abbrev.size > 1 || !is_valid_name(Abbrev)))
    throw "Argument abbreviations, if specified, must be a single non-whitespace character";

  if (meta.is_required.has_value()) {
    if (*meta.is_required && meta.default_value.has_value()) throw "Required arguments cannot have default values";
    if (!*meta.is_required && !meta.default_value.has_value()) throw "Optional arguments must have default values";
  }

  if (meta.action == Action::APPEND || meta.action == Action::CSV) {
    if (!concepts::Container<T>)
      throw "The APPEND and CSV actions require that the argument type satisfy the opz::concepts::Container concept";
    if (meta.implicit_value.has_value())
      throw "The APPEND and CSV actions do not work with implicit value since they require a value from the "
            "command-line";
  }
}

template <typename T>
constexpr void validate_pos(ArgMeta<T> const &meta) {
  if (meta.implicit_value.has_value())
    throw "Implicit value cannot be used with positionals because they always take an unidentified value from the "
          "command-line";
  if (meta.action == Action::COUNT)
    throw "The COUNT action cannot be used with positionals because they always take an unidentified value from the "
          "command-line";
  if (meta.action == Action::PRINT_HELP || meta.action == Action::PRINT_VERSION)
    throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not positionals because they "
          "always take a value from the command-line)";
}

template <typename T>
constexpr void validate_opt(ArgMeta<T> const &meta) {
  if (meta.action == Action::COUNT) {
    if (!concepts::Integer<T>) throw "The COUNT action only works with integer types";
    if (!meta.implicit_value.has_value()) throw "The COUNT action requires an implicit value";
  }
  if (meta.action == Action::PRINT_HELP || meta.action == Action::PRINT_VERSION)
    throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not options because they always "
          "take a value from the command-line)";
}

template <typename T>
constexpr void validate_flg(ArgMeta<T> const &meta) {
  if (meta.is_required.value_or(false)) throw "Flags cannot be required";
  if (!std::is_same_v<T, bool> && !meta.implicit_value.has_value())
    throw "Non-boolean flags require that the implicit value is specified";
  if (meta.action == Action::COUNT) {
    if (!concepts::Integer<T>) throw "The COUNT action only works with integer types";
    if (!meta.implicit_value.has_value()) throw "The COUNT action requires an implicit value";
  }
  // TODO(gather):
  // if (arg.type == ArgType::FLG && arg.gather_amount != 1) // 1 is the default
  //   throw "Flags cannot use gather because they do not take values from the command-line";
}

} // namespace opz

#endif // OPZIONI_ARG_H
