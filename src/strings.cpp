#include "opzioni/strings.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opz {

auto trim(std::string_view sv) noexcept -> std::string {
  auto const begin = sv.find_first_not_of(whitespace);
  auto const end = sv.find_last_not_of(whitespace);
  return {sv, begin, end + 1 - begin};
}

auto limit_within(std::span<std::string> words, std::size_t const max_width) noexcept
  -> std::vector<std::vector<std::string>> {
  std::size_t cur_max = max_width;
  std::vector<std::vector<std::string>> lines(1);
  for (auto const &word : words) {
    auto const trimmed_word = trim(word);
    if (trimmed_word.length() < cur_max) {
      lines.back().emplace_back(trimmed_word);
      cur_max -= (trimmed_word.length() + 1); // +1 for space in between
    } else {
      lines.emplace_back();
      lines.back().emplace_back(trimmed_word);
      cur_max = max_width - trimmed_word.length();
    }
  }
  return lines;
}

auto limit_within(std::string_view const text, std::size_t const max_width) noexcept
  -> std::vector<std::vector<std::string>> {
  std::vector<std::string> words;
  words.reserve(text.size()); // TODO: keep this reserve?
  for (auto const word : text | std::views::split(' ')) {
    words.emplace_back(word.begin(), std::ranges::distance(word));
  }
  return limit_within(std::span(words), max_width);
}

auto limit_string_within(std::string_view const text, std::size_t const max_width) noexcept -> std::string {
  auto const split_lines = limit_within(text, max_width);
  auto const lines = split_lines | std::views::transform([](auto const &words) { return fmt::join(words, " "); });
  return fmt::format("{}", fmt::join(lines, "\n"));
}

} // namespace opz
