#include "string.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opzioni {

std::string_view trim(std::string_view sv) noexcept {
  auto const begin = sv.find_first_not_of(whitespace);
  auto const end = sv.size() - sv.find_last_not_of(whitespace) - 1;
  sv.remove_prefix(std::min(begin, sv.size()));
  sv.remove_suffix(std::min(end, sv.size()));
  return sv;
}

auto limit_within(std::string_view const text, std::size_t const max_width) noexcept
    -> std::vector<std::vector<std::string_view>> {
  auto const range2str_view = [](auto const &r) { return std::string_view(&*r.begin(), std::ranges::distance(r)); };
  auto const words = text | std::views::split(' ') | std::views::transform(range2str_view);
  return limit_within(words, max_width);
}

std::string limit_string_within(std::string_view const text, std::size_t const max_width) noexcept {
  auto const split_lines = limit_within(text, max_width);
  auto const lines = split_lines | std::views::transform([](auto const &words) { return fmt::join(words, " "); });
  return fmt::format("{}", fmt::join(lines, "\n"));
}

} // namespace opzioni
