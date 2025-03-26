#ifndef OPZIONI_ARG_HPP
#define OPZIONI_ARG_HPP

#include <any>
#include <map>
#include <optional>
#include <string_view>

#include "concepts.hpp"
#include "fixed_string.hpp"

namespace opz {

enum struct ArgType { POS, OPT, FLG };
std::string_view to_string(ArgType const at) noexcept;

struct CmdInfo;

// +----------------------------------+
// |       forward declarations       |
// +----------------------------------+

class CmdInfoGetter;
template <typename> struct Arg;

namespace act {

template <concepts::Container C>
void append(
  std::map<std::string_view, std::any> &, Arg<C> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

template <typename T>
void assign(
  std::map<std::string_view, std::any> &, Arg<T> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

template <concepts::Integer I>
void count(
  std::map<std::string_view, std::any> &, Arg<I> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

template <concepts::Container C>
void csv(
  std::map<std::string_view, std::any> &, Arg<C> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

void print_help(
  std::map<std::string_view, std::any> &, Arg<bool> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

void print_version(
  std::map<std::string_view, std::any> &, Arg<bool> const &, std::optional<std::string_view> const &, CmdInfoGetter &
);

} // namespace act

template <typename T>
using ActionFn = void (*)(
  std::map<std::string_view, std::any> &args_map,
  Arg<T> const &arg,
  std::optional<std::string_view> const &value,
  CmdInfoGetter &info
);

// +---------------------------------+
// |             ArgMeta             |
// +---------------------------------+

template <typename T>
struct ArgMeta {
  std::string_view help{};
  std::optional<bool> is_required{};
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};
  ActionFn<T> action{act::assign};
};

constexpr static ArgMeta<bool> default_help = {
  .help = "Display this information",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
  .action = act::print_help,
};

constexpr static ArgMeta<bool> default_version = {
  .help = "Display {cmd_name}'s version",
  .is_required = false,
  .default_value = false,
  .implicit_value = true,
  .action = act::print_version,
};

// +---------------------------------+
// |               Arg               |
// +---------------------------------+

template <typename T>
struct Arg {
  using value_type = T;

  ArgType type{ArgType::POS};
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view help{};
  bool is_required{false};
  std::optional<T> default_value{};
  std::optional<T> implicit_value{};
  ActionFn<T> action{nullptr};

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

  if (meta.action == nullptr) {
    throw "The action function cannot be null";
  }

  if constexpr (concepts::Container<T>) {
    if (meta.implicit_value.has_value() && (meta.action == act::append<T> || meta.action == act::csv<T>))
      throw "The APPEND and CSV actions do not work with implicit value since they require a value from the "
            "command-line";
  }
}

template <typename T>
constexpr void validate_pos(ArgMeta<T> const &meta) {
  if (meta.implicit_value.has_value())
    throw "Implicit value cannot be used with positionals because they always take an unidentified value from the "
          "command-line";
  if constexpr (concepts::Integer<T>) {
    if (meta.action == act::count<T>)
      throw "The COUNT action cannot be used with positionals because they always take an unidentified value from the "
            "command-line";
  }
  if constexpr (std::is_same_v<T, bool>) {
    if (meta.action == act::print_help || meta.action == act::print_version)
      throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not positionals because they "
            "always take a value from the command-line)";
  }
}

template <typename T>
constexpr void validate_opt(ArgMeta<T> const &meta) {
  if constexpr (concepts::Integer<T>) {
    if (meta.action == act::count<T> && !meta.implicit_value.has_value())
      throw "The COUNT action requires an implicit value";
  }
  if constexpr (std::is_same_v<T, bool>) {
    if (meta.action == act::print_help || meta.action == act::print_version)
      throw "The PRINT_HELP and PRINT_VERSION actions can only be used with flags (and not options because they "
            "always take a value from the command-line)";
  }
}

template <typename T>
constexpr void validate_flg(ArgMeta<T> const &meta) {
  if (meta.is_required.value_or(false)) throw "Flags cannot be required";
  if (!std::is_same_v<T, bool> && !meta.implicit_value.has_value())
    throw "Non-boolean flags require that the implicit value is specified";
  if constexpr (concepts::Integer<T>) {
    if (meta.action == act::count<T> && !meta.implicit_value.has_value())
      throw "The COUNT action requires an implicit value";
  }
}

} // namespace opz

#endif // OPZIONI_ARG_HPP
