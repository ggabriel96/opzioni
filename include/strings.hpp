#ifndef OPZIONI_STRINGS_HPP
#define OPZIONI_STRINGS_HPP

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opz {

constexpr char nl = '\n';
constexpr std::string_view whitespace = " \f\n\r\t\v";

auto trim(std::string_view) noexcept -> std::string;
auto limit_within(std::span<std::string>, std::size_t const) noexcept -> std::vector<std::vector<std::string>>;
auto limit_within(std::string_view const, std::size_t const) noexcept -> std::vector<std::vector<std::string>>;
auto limit_string_within(std::string_view const, std::size_t const) noexcept -> std::string;

constexpr bool is_valid_name(std::string_view name) noexcept {
  return !name.empty() && !std::ranges::any_of(name, [](char const ch) { return whitespace.contains(ch); });
}

constexpr bool is_valid_intro(std::string_view intro) noexcept {
  return !intro.empty() && !whitespace.contains(intro.front()) && !whitespace.contains(intro.back());
}

} // namespace opz

#endif // OPZIONI_STRINGS_HPP
