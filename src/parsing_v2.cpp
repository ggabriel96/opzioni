#include "opzioni/parsing_v2.hpp"

namespace opz {

std::string_view to_string(TokenType type) noexcept {
  switch (type) {
    case TokenType::PROG_NAME: return "PROG_NAME";
    case TokenType::DASH: return "DASH";
    case TokenType::DASH_DASH: return "DASH_DASH";
    case TokenType::FLG_LONG: return "FLG_LONG";
    case TokenType::FLG_MANY: return "FLG_MANY";
    case TokenType::FLG_SHORT: return "FLG_SHORT";
    case TokenType::OPT_LONG_AND_VALUE: return "OPT_LONG_AND_VALUE";
    case TokenType::OPT_LONG: return "OPT_LONG";
    case TokenType::OPT_SHORT_AND_VALUE: return "OPT_SHORT_AND_VALUE";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::END: return "END";
    default: return "ERR";
  }
}

// +-----------------------------------------+
// |                 Scanner                 |
// +-----------------------------------------+

/* public */

std::vector<Token> Scanner::operator()() noexcept {
  this->add_token(TokenType::PROG_NAME);
  for (this->args_idx = 1; this->args_idx < this->args.size(); ++this->args_idx) {
    this->cur_col = 0;
    this->scan_token();
  }
  // don't use add_token() because this->args_idx is not valid anymore
  this->tokens.emplace_back(TokenType::END, "", std::nullopt, this->args_idx);
  return this->tokens;
}

/* private */

[[nodiscard]] inline std::string_view const &Scanner::cur_arg() const noexcept { return this->args[this->args_idx]; }

[[nodiscard]] bool Scanner::is_cur_end() const noexcept { return this->cur_col >= this->cur_arg().size(); }

[[nodiscard]] auto Scanner::advance() noexcept { return this->cur_arg()[this->cur_col++]; }

[[nodiscard]] char Scanner::peek() const noexcept {
  if (this->is_cur_end()) return '\0';
  return this->cur_arg()[this->cur_col];
}

void Scanner::consume() noexcept { this->cur_col += 1; }

[[nodiscard]] bool Scanner::match(char expected) noexcept {
  // if (this->is_cur_end()) return false;
  // if (this->cur_arg()[this->current_col] != expected) return false;
  if (this->peek() != expected) return false;
  this->cur_col += 1;
  return true;
}

void Scanner::add_token(TokenType type, std::optional<std::string_view> value) noexcept {
  this->tokens.emplace_back(type, this->cur_arg(), value, this->args_idx);
}

void Scanner::scan_token() noexcept {
  if (this->peek() != dash) {
    this->add_token(TokenType::IDENTIFIER, this->cur_arg());
    return;
  }

  this->consume();
  if (this->is_cur_end()) this->add_token(TokenType::DASH);
  else if (this->match(dash)) {
    if (this->is_cur_end()) this->add_token(TokenType::DASH_DASH);
    else this->try_long_opt();
  } else this->try_short_opt();
}

void Scanner::try_long_opt() noexcept { this->add_token(TokenType::OPT_LONG); }

void Scanner::try_short_opt() noexcept { this->add_token(TokenType::FLG_SHORT); }

} // namespace opz
