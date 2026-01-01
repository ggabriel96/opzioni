#include "opzioni/strings.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace opz {
// +------------------------------------------------+
// |                Paragraph & Line                |
// +------------------------------------------------+

void Line::add_word(std::string_view const word) { this->_words.emplace_back(word); }

[[nodiscard]] std::vector<std::string> const &Line::words() const noexcept { return this->_words; }

Paragraph::Paragraph(std::size_t const max_width) : max_width(max_width), width_left(max_width), _lines(1) {}

void Paragraph::add_word(std::string_view const word) {
  if (word.length() < this->width_left) {
    this->_lines.back().add_word(word);
    this->width_left -= (word.length() + 1); // +1 for space in between
  } else {
    this->_lines.emplace_back();
    this->_lines.back().add_word(word);
    this->width_left = this->max_width - word.length();
  }
}

[[nodiscard]] std::vector<Line> const &Paragraph::lines() const noexcept { return this->_lines; }

[[nodiscard]] std::string Paragraph::to_str_lines() const {
  auto const str_lines =
    this->_lines | std::views::transform([](auto const &line) { return fmt::join(line.words(), " "); });
  return fmt::format("{}", fmt::join(str_lines, "\n"));
}

// +------------------------------------------------+
// |                Helper functions                |
// +------------------------------------------------+

Paragraph limit_within(std::span<std::string const> const words, std::size_t const max_width) noexcept {
  Paragraph paragraph(max_width);
  for (auto const &word : words) {
    paragraph.add_word(word);
  }
  return paragraph;
}

Paragraph limit_line_within(std::string_view line, std::size_t const max_width) noexcept {
  Paragraph paragraph(max_width);
  for (auto const word : line | std::views::split(' ')) {
    paragraph.add_word({word.begin(), word.end()});
  }
  return paragraph;
}

} // namespace opz
