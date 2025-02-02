#ifndef OPZIONI_ARG_H
#define OPZIONI_ARG_H

#include <cstddef>
#include <string_view>

struct ArgMeta {
  std::string_view help{};
  bool is_required = true;
};

enum struct ArgType { POS, OPT, FLG };

struct Arg {
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  std::string_view help{};
  bool is_required = true;
  //   BuiltinVariant default_value{};
  //   BuiltinVariant implicit_value{};
  //   act::fn::Signature action_fn = act::fn::assign<std::string_view>;
  //   std::size_t gather_amount = 1;
  //   DefaultValueSetter default_setter = nullptr;

  constexpr bool has_abbrev() const noexcept { return !abbrev.empty(); }
  //   constexpr bool has_default() const noexcept { return default_value.index() != 0 || default_setter != nullptr; }
  //   constexpr bool has_implicit() const noexcept { return implicit_value.index() != 0; }
  constexpr bool is_positional() const noexcept { return type == ArgType::POS; }

  std::string format_base_usage() const noexcept;
  std::string format_for_help_description() const noexcept;
  std::string format_for_help_index() const noexcept;
  std::string format_for_usage_summary() const noexcept;
};

constexpr bool operator<(Arg const &lhs, Arg const &rhs) noexcept {
  bool const lhs_is_positional = lhs.type == ArgType::POS;
  bool const rhs_is_positional = rhs.type == ArgType::POS;

  if (lhs_is_positional && rhs_is_positional)
    return false; // don't move positionals relative to each other

  if (!lhs_is_positional && !rhs_is_positional)
    return lhs.name < rhs.name; // sort non-positionals by name

  return lhs_is_positional; // sort positionals before other types
}

constexpr bool operator==(Arg const &lhs, Arg const &rhs) noexcept {
  auto const same_name = lhs.name == rhs.name;
  auto const same_abbrev = lhs.has_abbrev() && rhs.has_abbrev() && lhs.abbrev == rhs.abbrev;
  return same_name || same_abbrev;
}

#endif