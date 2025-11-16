#include "opzioni/scanner.hpp"

namespace opz {

std::string_view to_string(TokenType type) noexcept {
  switch (type) {
    case TokenType::PROG_NAME: return "PROG_NAME";
    case TokenType::DASH: return "DASH";
    case TokenType::DASH_DASH: return "DASH_DASH";
    case TokenType::FLG: return "FLG";
    case TokenType::OPT_OR_FLG_LONG: return "OPT_OR_FLG_LONG";
    case TokenType::OPT_LONG_AND_VALUE: return "OPT_LONG_AND_VALUE";
    case TokenType::OPT_SHORT_AND_VALUE: return "OPT_SHORT_AND_VALUE";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    default: return "ERR";
  }
}

// +-----------------------------------------+
// |                 Scanner                 |
// +-----------------------------------------+

/* public */

std::vector<Token> Scanner::operator()() noexcept {
  this->add_token(TokenType::PROG_NAME, std::nullopt, this->cur_arg());
  for (this->args_idx = 1; this->args_idx < this->args.size(); ++this->args_idx) {
    this->cur_col = 0;
    this->scan_token();
  }
  return this->tokens;
}

/* private */

[[nodiscard]] inline std::string_view const &Scanner::cur_arg() const noexcept { return this->args[this->args_idx]; }

[[nodiscard]] bool Scanner::is_cur_end() const noexcept { return this->cur_col >= this->cur_arg().size(); }

[[nodiscard]] auto Scanner::advance() noexcept { return this->cur_arg().substr(this->cur_col++, 1); }

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

void Scanner::add_token(TokenType type, std::optional<std::string_view> name, std::optional<std::string_view> value) noexcept {
  this->tokens.emplace_back(type, this->args_idx, name, value);
}

void Scanner::scan_token() noexcept {
  if (!this->match(dash)) {
    this->add_token(TokenType::IDENTIFIER, std::nullopt, this->cur_arg());
    return;
  }

  if (this->is_cur_end()) this->add_token(TokenType::DASH);
  else if (this->match(dash)) {
    if (this->is_cur_end()) this->add_token(TokenType::DASH_DASH);
    else this->long_opt();
  } else this->short_opt();
}

void Scanner::long_opt() noexcept {
  // -- was already consumed
  // look for either: name=value or name
  while (!this->is_cur_end() && this->peek() != '=') consume();
  if (this->match('=')) {
    // start at 2 to discard initial dash dash; -3 would be -1 but adds 2 because we start at 2
    auto const name = this->cur_arg().substr(2, this->cur_col - 3);
    auto const value = this->cur_arg().substr(this->cur_col);
    this->add_token(TokenType::OPT_LONG_AND_VALUE, name, value);
  } else {
    this->add_token(TokenType::OPT_OR_FLG_LONG, this->cur_arg().substr(2)); // 2 to discard initial dash dash
  }
}

void Scanner::short_opt() noexcept {
  // - was already consumed
  // look for either: -Ovalue or -f or -xpto
  if (auto const c = this->peek(); c >= 'A' && c <= 'Z') {
    auto const name = this->advance();
    auto const value = this->cur_arg().substr(this->cur_col);
    this->add_token(TokenType::OPT_SHORT_AND_VALUE, name, value);
    return;
  }

  if (this->cur_arg().length() == 2) {
    this->add_token(TokenType::FLG, this->advance());
    return;
  }

  while (!this->is_cur_end()) {
    this->add_token(TokenType::FLG, this->advance());
  }
}

} // namespace opz
