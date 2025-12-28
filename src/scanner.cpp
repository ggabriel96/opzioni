#include "opzioni/scanner.hpp"

namespace opz {

std::string_view to_string(TokenKind const kind) noexcept {
  switch (kind) {
    case TokenKind::PROG_NAME: return "PROG_NAME";
    case TokenKind::DASH_DASH: return "DASH_DASH";
    case TokenKind::FLG: return "FLG";
    case TokenKind::OPT_OR_FLG_LONG: return "OPT_OR_FLG_LONG";
    case TokenKind::OPT_LONG_AND_VALUE: return "OPT_LONG_AND_VALUE";
    case TokenKind::OPT_SHORT_AND_VALUE: return "OPT_SHORT_AND_VALUE";
    case TokenKind::IDENTIFIER: return "IDENTIFIER";
    default: return "ERR";
  }
}

TokenIndices index_tokens(std::span<Token const> const tokens) {
  auto indices = TokenIndices();
  if (!tokens.empty()) { // should never happen
    for (std::size_t index = 1; index < tokens.size(); ++index) {
      switch (auto const &tok = tokens[index]; tok.kind) {
        case TokenKind::PROG_NAME: break;
        case TokenKind::DASH_DASH:
          // +1 to ignore the dash-dash
          for (std::size_t offset = index + 1; offset < tokens.size(); ++offset) {
            indices.positionals.push_back(offset);
          }
          index = tokens.size(); // break below refers to the switch, not the for loop
          break;
        case TokenKind::FLG: [[fallthrough]];
        case TokenKind::OPT_OR_FLG_LONG: [[fallthrough]];
        case TokenKind::OPT_LONG_AND_VALUE: [[fallthrough]];
        case TokenKind::OPT_SHORT_AND_VALUE: indices.opts_n_flgs[*tok.name].push_back(index); break;
        case TokenKind::IDENTIFIER: indices.positionals.push_back(index); break;
      }
    }
  }
  return indices;
}

// +-----------------------------------------+
// |                 Scanner                 |
// +-----------------------------------------+

/* public */

std::vector<Token> Scanner::operator()() noexcept {
  this->add_token(TokenKind::PROG_NAME, std::nullopt, this->cur_arg());
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

[[nodiscard]] bool Scanner::match(char const expected) noexcept {
  // if (this->is_cur_end()) return false;
  // if (this->cur_arg()[this->current_col] != expected) return false;
  if (this->peek() != expected) return false;
  this->cur_col += 1;
  return true;
}

void Scanner::add_token(
  TokenKind kind, std::optional<std::string_view> name, std::optional<std::string_view> value
) noexcept {
  this->tokens.emplace_back(kind, this->args_idx, name, value);
}

void Scanner::scan_token() noexcept {
  if (!this->match(dash)) {
    this->add_token(TokenKind::IDENTIFIER, std::nullopt, this->cur_arg());
    return;
  }

  if (this->is_cur_end()) this->add_token(TokenKind::IDENTIFIER, std::nullopt, this->cur_arg());
  else if (this->match(dash)) {
    if (this->is_cur_end()) this->add_token(TokenKind::DASH_DASH);
    else this->long_opt();
  } else this->short_opt();
}

void Scanner::long_opt() noexcept {
  // -- was already consumed
  // look for either: name=value or name
  while (!this->is_cur_end() && this->peek() != '=')
    consume();
  if (this->match('=')) {
    // start at 2 to discard initial dash-dash; -3 would be -1 but adds 2 because we start at 2
    auto const name = this->cur_arg().substr(2, this->cur_col - 3);
    auto const value = this->cur_arg().substr(this->cur_col);
    this->add_token(TokenKind::OPT_LONG_AND_VALUE, name, value);
  } else {
    this->add_token(TokenKind::OPT_OR_FLG_LONG, this->cur_arg().substr(2)); // 2 to discard initial dash-dash
  }
}

void Scanner::short_opt() noexcept {
  // - was already consumed
  // look for either: -Ovalue or -O value or -f or -xpto
  if (auto const c = this->peek(); c >= 'A' && c <= 'Z') {
    auto const name = this->advance();
    std::optional<std::string_view> value = std::nullopt;
    if (this->cur_arg().length() > 2)
      value.emplace(std::next(this->cur_arg().begin(), this->cur_col), this->cur_arg().end());
    this->add_token(TokenKind::OPT_SHORT_AND_VALUE, name, value);
    return;
  }

  if (this->cur_arg().length() == 2) {
    this->add_token(TokenKind::FLG, this->advance());
    return;
  }

  while (!this->is_cur_end()) {
    this->add_token(TokenKind::FLG, this->advance());
  }
}

} // namespace opz
