#ifndef OPZIONI_PARSING_V2_HPP
#define OPZIONI_PARSING_V2_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace opz {

constexpr static auto dash = '-';

enum struct TokenType {
  PROG_NAME,
  DASH,                // -
  DASH_DASH,           // --
  FLG,                 // -f (-xpto adds many of this)
  OPT_OR_FLG_LONG,     // --option or --flag
  OPT_LONG_AND_VALUE,  // --option=value
  OPT_SHORT_AND_VALUE, // -Ovalue
  IDENTIFIER,          // positional, command, value after OPT_OR_FLG_LONG
  END,
};

struct Token {
  TokenType type;
  std::uint32_t args_idx;
  std::optional<std::string_view> name;
  std::optional<std::string_view> value;
};

std::string_view to_string(TokenType type) noexcept;

class Scanner {
public:
  Scanner(std::span<char const *> args) {
    this->tokens.reserve(args.size());
    this->args.reserve(args.size());
    for (char const *a : args) {
      this->args.emplace_back(a);
    }
  }

  Scanner(int argc, char const *argv[]) : Scanner(std::span{argv, static_cast<std::size_t>(argc)}) {}

  std::vector<Token> operator()() noexcept;

private:
  std::vector<std::string_view> args;

  std::vector<Token> tokens;
  std::uint32_t args_idx = 0;
  std::uint32_t cur_col = 0;

  [[nodiscard]] inline std::string_view const &cur_arg() const noexcept;
  [[nodiscard]] bool is_cur_end() const noexcept;
  [[nodiscard]] auto advance() noexcept;
  [[nodiscard]] char peek() const noexcept;
  void consume() noexcept;
  [[nodiscard]] bool match(char expected) noexcept;

  void add_token(TokenType type, std::optional<std::string_view> name = std::nullopt, std::optional<std::string_view> value = std::nullopt) noexcept;
  void scan_token() noexcept;
  void long_opt() noexcept;
  void short_opt() noexcept;
};

} // namespace opz

#endif // OPZIONI_PARSING_V2_HPP
