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

class Line {
public:

  void add_word(std::string_view);
  [[nodiscard]] std::vector<std::string> const &words() const noexcept;

private:

  std::vector<std::string> _words;
};

class Paragraph {
public:

  explicit Paragraph(std::size_t);

  void add_word(std::string_view);
  [[nodiscard]] std::vector<Line> const &lines() const noexcept;
  [[nodiscard]] std::string to_str_lines() const;

private:

  std::size_t max_width;
  std::size_t width_left;
  std::vector<Line> _lines;
};

Paragraph limit_within(std::span<std::string const>, std::size_t) noexcept;
Paragraph limit_line_within(std::string_view, std::size_t) noexcept;

constexpr bool is_valid_name(std::string_view name) noexcept {
  return !name.empty() && !std::ranges::any_of(name, [](char const ch) { return whitespace.contains(ch); });
}

constexpr bool is_valid_intro(std::string_view intro) noexcept {
  return !intro.empty() && !whitespace.contains(intro.front()) && !whitespace.contains(intro.back());
}

} // namespace opz

#endif // OPZIONI_STRINGS_HPP
