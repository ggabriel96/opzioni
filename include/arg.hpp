#ifndef OPZIONI_ARG_H
#define OPZIONI_ARG_H

#include <optional>
#include <string_view>

#include "converters.hpp"

namespace opz {

enum struct ArgType { POS, OPT, FLG };

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

std::string_view to_string(ArgType const at) noexcept;

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

  [[nodiscard]] constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  [[nodiscard]] constexpr bool has_default() const noexcept { return default_value.has_value(); }
  [[nodiscard]] constexpr bool has_implicit() const noexcept { return implicit_value.has_value(); }
};

// +---------------------------------+
// |             ArgView             |
// +---------------------------------+

struct ArgView {
  std::size_t tuple_idx{};
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  bool is_required = false;
  bool has_implicit = false;

  template <typename T>
  ArgView(std::size_t tuple_idx, Arg<T> const &other)
      : tuple_idx(tuple_idx),
        type(other.type),
        name(other.name),
        abbrev(other.abbrev),
        is_required(other.is_required),
        has_implicit(other.implicit_value.has_value()) {}
};

} // namespace opz

#endif // OPZIONI_ARG_H
