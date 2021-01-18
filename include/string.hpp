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
constexpr std::string_view valid_name_chars = "0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ-abcdefghijklmnopqrstuvwxyz";

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

constexpr bool contains(std::string_view const str, char const ch) noexcept {
  return str.find(ch) != std::string_view::npos;
}

constexpr bool is_valid_name(std::string_view name) noexcept {
  return std::ranges::find_if(name, [](char const ch) { return !contains(valid_name_chars, ch); }) == name.end();
}

} // namespace opzioni

#endif // OPZIONI_STRING_H
