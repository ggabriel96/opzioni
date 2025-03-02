#include "string.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opz {

std::string_view trim(std::string_view sv) noexcept {
  auto const begin = sv.find_first_not_of(whitespace);
  auto const end = sv.size() - sv.find_last_not_of(whitespace) - 1;
  sv.remove_prefix(std::min(begin, sv.size()));
  sv.remove_suffix(std::min(end, sv.size()));
  return sv;
}

auto limit_within(std::span<std::string_view> words, std::size_t const max_width) noexcept
  -> std::vector<std::vector<std::string_view>> {
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

auto limit_within(std::string_view const text, std::size_t const max_width) noexcept
  -> std::vector<std::vector<std::string_view>> {
  auto const range2str_view = [](auto const &r) { return std::string_view(&*r.begin(), std::ranges::distance(r)); };
  std::vector<std::string_view> words;
  words.reserve(text.size());
  for (auto const word : text | std::views::split(' ')) {
    words.push_back(range2str_view(word));
  }
  return limit_within(std::span(words), max_width);
}

std::string limit_string_within(std::string_view const text, std::size_t const max_width) noexcept {
  auto const split_lines = limit_within(text, max_width);
  auto const lines = split_lines | std::views::transform([](auto const &words) { return fmt::join(words, " "); });
  return fmt::format("{}", fmt::join(lines, "\n"));
}

} // namespace opz
