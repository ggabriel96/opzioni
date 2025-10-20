#ifndef OPZIONI_ARG_HPP
#define OPZIONI_ARG_HPP

#include <any>
#include <map>
#include <optional>
#include <string_view>

#include "opzioni/concepts.hpp"
#include "opzioni/fixed_string.hpp"

namespace opz {

enum struct ArgType { POS, OPT, FLG };
std::string_view to_string(ArgType const at) noexcept;

// +----------------------------------+
// |       forward declarations       |
// +----------------------------------+

template <typename, typename> struct Arg;

namespace act {

struct append {};
struct assign {};
struct count {};
struct csv {};
struct print_help {};
struct print_version {};

} // namespace act

// +---------------------------------+
// |             ArgMeta             |
// +---------------------------------+

template <typename T, typename Tag = act::assign>
struct ArgMeta {
  std::string_view help{};
  std::optional<bool> is_required{};
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};
};

constexpr static ArgMeta<bool, act::print_help> default_help = {
  .help = "Display this information",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
};

constexpr static ArgMeta<bool, act::print_version> default_version = {
  .help = "Display {cmd_name}'s version",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
};

// +---------------------------------+
// |               Arg               |
// +---------------------------------+

template <typename T, typename Tag = act::assign>
struct Arg {
  using value_type = T;
  using tag_type = Tag;

  ArgType type{ArgType::POS};
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view help{};
  bool is_required{false};
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};

  [[nodiscard]] constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  [[nodiscard]] constexpr bool has_default() const noexcept { return default_value.has_value(); }
  [[nodiscard]] constexpr bool has_implicit() const noexcept { return implicit_value.has_value(); }
};

// +---------------------------------+
// |       ArgMeta validations       |
// +---------------------------------+

template <FixedString Name, FixedString Abbrev, typename T, typename Tag>
constexpr void validate_common(ArgMeta<T, Tag> const &meta) {
  if (!is_valid_name(Name)) throw "Argument names must neither be empty nor contain any whitespace";
  if (Abbrev.size != 0 && (Abbrev.size > 1 || !is_valid_name(Abbrev)))
    throw "Argument abbreviations, if specified, must be a single non-whitespace character";

  if (meta.is_required.has_value()) {
    if (*meta.is_required && meta.default_value.has_value()) throw "Required arguments cannot have default values";
    if (!*meta.is_required && !meta.default_value.has_value()) throw "Optional arguments must have default values";
  }

  if constexpr (concepts::Container<T>) {
    if (meta.implicit_value.has_value() && (std::is_same_v<Tag, act::append> || std::is_same_v<Tag, act::csv>))
      throw "The APPEND and CSV actions do not work with implicit value since they require a value from the command-line";
  }
}

template <typename T, typename Tag>
constexpr void validate_pos(ArgMeta<T, Tag> const &meta) {
  if (meta.implicit_value.has_value())
    throw "Implicit value cannot be used with positionals because they always take a value from the command-line";
  if constexpr (concepts::Integer<T>) {
    if (std::is_same_v<Tag, act::count>)
      throw "The COUNT action can only be used with flags because they count how many times an argument was provided; since a positional always takes a value from the command-line, it would be wrongly ignored";
  }
  if constexpr (std::is_same_v<T, bool>) {
    if (std::is_same_v<Tag, act::print_help> || std::is_same_v<Tag, act::print_version>)
      throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not positionals because they always take a value from the command-line)";
  }
}

template <typename T, typename Tag>
constexpr void validate_opt(ArgMeta<T, Tag> const &meta) {
  if constexpr (concepts::Integer<T>) {
    if (std::is_same_v<Tag, act::count>)
      throw "The COUNT action can only be used with flags because they count how many times an argument was provided; since an option might take a value from the command-line, it would be wrongly ignored";
  }
  if constexpr (std::is_same_v<T, bool>) {
    if (std::is_same_v<Tag, act::print_help> || std::is_same_v<Tag, act::print_version>)
      throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not options because they might take a value from the command-line)";
  }
}

template <typename T, typename Tag>
constexpr void validate_flg(ArgMeta<T, Tag> const &meta) {
  if (meta.is_required.value_or(false)) throw "Flags cannot be required";
  if (!std::is_same_v<T, bool> && !meta.implicit_value.has_value())
    throw "Non-boolean flags require that the implicit value is specified";
  if constexpr (concepts::Integer<T>) {
    if (std::is_same_v<Tag, act::count> && !meta.implicit_value.has_value())
      throw "The COUNT action requires an implicit value";
  } else if (std::is_same_v<Tag, act::count>) {
    throw "The COUNT action cannot be used with non-integer types";
  }
}

} // namespace opz

#endif // OPZIONI_ARG_HPP
