#ifndef OPZIONI_ARG_H
#define OPZIONI_ARG_H

#include <optional>
#include <string_view>

namespace opz {

template <typename T>
struct ArgMeta {
  std::string_view help{};
  std::optional<bool> is_required{};
  std::optional<T> default_value{};
};

enum struct ArgType { POS, OPT, FLG };

std::string_view ToString(ArgType at) noexcept;

template <typename T>
struct Arg {
  using value_type = T;

  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view help{};
  bool is_required = false;
  std::optional<T> default_value{};
  //   BuiltinVariant implicit_value{};
  //   act::fn::Signature action_fn = act::fn::assign<std::string_view>;
  //   std::size_t gather_amount = 1;
  //   DefaultValueSetter default_setter = nullptr;

  constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  constexpr bool has_default() const noexcept { return default_value.has_value(); }
  //   constexpr bool has_implicit() const noexcept { return implicit_value.index() != 0; }
  constexpr bool is_positional() const noexcept { return type == ArgType::POS; }

  std::string format_base_usage() const noexcept;
  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;
};

template <typename T, typename U>
constexpr bool operator<(Arg<T> const &lhs, Arg<U> const &rhs) noexcept {
  bool const lhs_is_positional = lhs.type == ArgType::POS;
  bool const rhs_is_positional = rhs.type == ArgType::POS;

  if (lhs_is_positional && rhs_is_positional)
    return false; // don't move positionals relative to each other

  if (!lhs_is_positional && !rhs_is_positional)
    return lhs.name < rhs.name; // sort non-positionals by name

  return lhs_is_positional; // sort positionals before other types
}

template <typename T, typename U>
constexpr bool operator==(Arg<T> const &lhs, Arg<U> const &rhs) noexcept {
  auto const same_name = lhs.name == rhs.name;
  auto const same_abbrev = lhs.has_abbrev() && rhs.has_abbrev() && lhs.abbrev == rhs.abbrev;
  return same_name || same_abbrev;
}

struct ArgView {
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  bool is_required = false;

  template <typename T>
  ArgView(Arg<T> const &other)
    : type(other.type), name(other.name), abbrev(other.abbrev), is_required(other.is_required) {}
};

} // namespace opz

#endif // OPZIONI_ARG_H
