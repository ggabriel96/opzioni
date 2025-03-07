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
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  bool is_required = false;
  bool has_implicit = false;

  template <typename T>
  ArgView(Arg<T> const &other)
      : type(other.type),
        name(other.name),
        abbrev(other.abbrev),
        is_required(other.is_required),
        has_implicit(other.implicit_value.has_value()) {}
};

} // namespace opz

#endif // OPZIONI_ARG_H
