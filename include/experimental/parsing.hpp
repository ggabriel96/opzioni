#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <any>
#include <map>
#include <span>
#include <string>
#include <string_view>

#include "converters.hpp"
#include "exceptions.hpp"
#include "experimental/fixed_string.hpp"
#include "experimental/program.hpp"
#include "experimental/string_list.hpp"
#include "experimental/type_list.hpp"

struct ParsedOption {
  std::string_view name;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;

  bool is_valid() const noexcept {
    return !name.empty();
  }
};

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view get_if_short_flags(std::string_view const) noexcept;
constexpr std::string_view get_if_long_flag(std::string_view const) noexcept;
constexpr ParsedOption try_parse_option(std::string_view const) noexcept;

#endif