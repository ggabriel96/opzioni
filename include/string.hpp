#ifndef OPZIONI_STRING_H
#define OPZIONI_STRING_H

#include <algorithm>
#include <concepts>
#include <string>
#include <string_view>
#include <vector>

namespace opzioni {

constexpr char nl = '\n';
constexpr std::string_view whitespace = " \f\n\r\t\v";

std::string_view trim(std::string_view) noexcept;

auto limit_within(std::ranges::range auto const &words, std::size_t const max_width) noexcept
    -> std::vector<std::vector<std::string_view>> requires(
        std::convertible_to<std::ranges::range_value_t<decltype(words)>, std::string_view>) {
  std::size_t cur_max = max_width;
  std::vector<std::vector<std::string_view>> lines(1);
  for (auto const &_word : words) {
    auto const word = trim(_word);
    if (word.length() < cur_max) {
      lines.back().push_back(word);
      cur_max -= (word.length() + 1); // +1 for space in between
    } else {
      lines.push_back({word});
      cur_max = max_width - word.length();
    }
  }
  return lines;
}

auto limit_within(std::string_view const, std::size_t const) noexcept -> std::vector<std::vector<std::string_view>>;

std::string limit_string_within(std::string_view const, std::size_t const) noexcept;

constexpr bool is_alphabetic(char const ch) noexcept { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

constexpr bool is_numeric(char const ch) noexcept { return ch >= '0' && ch <= '9'; }

constexpr bool is_valid_name(std::string_view name) noexcept {
  return name.length() > 0 && is_alphabetic(name.front()) &&
         (is_alphabetic(name.back()) || (name.length() > 1 && is_numeric(name.back()))) &&
         std::ranges::all_of(
             name, [](char const ch) { return is_alphabetic(ch) || is_numeric(ch) || ch == '-' || ch == '_'; });
}

constexpr bool is_valid_abbrev(std::string_view abbrev) noexcept {
  return abbrev.length() == 0 || (abbrev.length() == 1 && (is_alphabetic(abbrev[0]) || is_numeric(abbrev[0])));
}

} // namespace opzioni

#endif // OPZIONI_STRING_H
