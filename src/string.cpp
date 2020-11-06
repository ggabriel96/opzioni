#include "string.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opzioni {

std::vector<std::vector<std::string_view>> limit_within(std::string const &text, std::size_t const max_width,
                                                        std::size_t const margin_left) noexcept {
  auto const range2str_view = [](auto const &r) { return std::string_view(&*r.begin(), std::ranges::distance(r)); };
  auto const words = text | std::views::split(' ') | std::views::transform(range2str_view);
  return limit_within(words, max_width, margin_left);
}

std::vector<std::vector<std::string_view>> limit_within(std::string const &text, std::size_t const max_width) noexcept {
  return limit_within(text, max_width, 0);
}

std::string limit_string_within(std::string const &text, std::size_t const max_width) {
  auto const split_lines = limit_within(text, max_width);
  auto const lines = split_lines | std::views::transform([](auto const &words) { return fmt::join(words, " "); });
  return fmt::format("{}", fmt::join(lines, "\n"));
}

} // namespace opzioni
