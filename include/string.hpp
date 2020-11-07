
#ifndef OPZIONI_STRING_H
#define OPZIONI_STRING_H

#include <string>
#include <string_view>
#include <vector>

namespace opzioni {

constexpr char nl = '\n';
constexpr std::string_view whitespace = " \f\n\r\t\v";

std::string_view trim(std::string_view) noexcept;

std::vector<std::vector<std::string_view>> limit_within(auto const &words, std::size_t const max_width,
                                                        std::size_t const margin_left) noexcept {
  std::size_t cur_max = max_width - margin_left;
  std::vector<std::vector<std::string_view>> lines(1);
  for (auto const &_word : words) {
    auto const word = trim(_word);
    if (word.length() < cur_max) {
      lines.back().push_back(word);
      cur_max -= (word.length() + 1); // +1 for space in between
    } else {
      lines.push_back({word});
      cur_max = max_width - margin_left - word.length();
    }
  }
  return lines;
}

std::vector<std::vector<std::string_view>> limit_within(std::string const &, std::size_t const,
                                                        std::size_t const) noexcept;

std::vector<std::vector<std::string_view>> limit_within(std::string const &, std::size_t const) noexcept;

std::string limit_string_within(std::string const &, std::size_t const);

} // namespace opzioni

#endif // OPZIONI_STRING_H