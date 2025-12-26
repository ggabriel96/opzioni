#ifndef OPZIONI_PARSING_HPP
#define OPZIONI_PARSING_HPP

#include <cstddef>
#include <functional>
#include <set>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "opzioni/actions.hpp"
#include "opzioni/arg.hpp"
#include "opzioni/args_map.hpp"
#include "opzioni/cmd_fmt.hpp"
#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/scanner.hpp"

namespace opz {

// TODO: is ArgView still necessary?
struct ArgView {
  std::size_t tuple_idx{};
  ArgType type = ArgType::POS;
  std::string_view name{};
  std::string_view abbrev{};
  bool is_required = false;
  bool has_implicit = false;

  template <typename T, typename Tag>
  constexpr static ArgView from(std::size_t const tuple_idx, Arg<T, Tag> const &other) {
    return ArgView{
      .tuple_idx = tuple_idx,
      .type = other.type,
      .name = other.name,
      .abbrev = other.abbrev,
      .is_required = other.is_required,
      .has_implicit = other.implicit_value.has_value()
    };
  }
};

template <concepts::Cmd... Cmds>
constexpr int
find_cmd(std::tuple<std::reference_wrapper<Cmds const> const...> const haystack, std::string_view const name) {
  // clang-format off
  return std::apply(
    [name](auto &&...elem) {
      int idx = 0, ret = -1;
      (void) // cast to void to suppress unused warning
      ((elem.get().name == name ? (ret = idx, true) : (++idx, false)) || ...);
      return ret;
    },
    haystack);
  // clang-format on
}

// +-----------------------+
// |       CmdParser       |
// +-----------------------+

template <concepts::Cmd Cmd>
class CmdParser {
public:

  using cmd_type = Cmd;

  std::reference_wrapper<Cmd const> cmd_ref;
  ExtraInfo extra_info;

  explicit CmdParser(Cmd const &cmd) : cmd_ref(cmd) {}

  ArgsMap<Cmd const> operator()(std::span<char const *> const args) {
    auto scanner = Scanner(args);
    auto const tokens = scanner();
    auto const indexes = index_tokens(tokens);
    // TODO: check we did not receive unknown arguments (throw UnknownArgument)
    auto map = this->get_args_map(tokens, indexes, 0, -1);
    return map;
  }

  ArgsMap<Cmd const> operator()(int const argc, char const *argv[]) {
    auto const args = std::span{argv, static_cast<std::size_t>(argc)};
    return (*this)(args);
  }

private:

  template <concepts::Cmd>
  friend class CmdParser;

  std::set<std::size_t> indexes_used_as_opt_values;

  CmdParser(Cmd const &cmd, ExtraInfo const &extra_info, std::string_view parent_cmd_name) : cmd_ref(cmd) {
    this->extra_info.parent_cmds_names.reserve(extra_info.parent_cmds_names.size() + 1);
    for (auto const name : extra_info.parent_cmds_names) {
      this->extra_info.parent_cmds_names.push_back(name);
    }
    this->extra_info.parent_cmds_names.push_back(parent_cmd_name);
  }

  auto get_cmd_fmt() const noexcept { return CmdFmt(this->cmd_ref.get(), this->extra_info); }

  auto get_args_map(
    std::span<Token const> const tokens,
    TokensIndexes const &indexes,
    std::size_t const recursion_start_idx,
    std::size_t recursion_end_idx
  ) {
    constexpr auto args_size = std::tuple_size_v<decltype(this->cmd_ref.get().args)>;
    auto args_map = ArgsMap<Cmd const>();
    args_map.exec_path = *tokens[recursion_start_idx].value;
    if (this->cmd_ref.get().has_subcmds())
      this->parse_possible_subcmd(args_map, tokens, indexes, recursion_start_idx, recursion_end_idx);
    // further args have to be > recursion_start_idx and <= recursion_end_idx
    this->process_tokens(
      args_map, tokens, indexes, recursion_start_idx, recursion_end_idx, std::make_index_sequence<args_size>()
    );
    return args_map;
  }

  void parse_possible_subcmd(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokensIndexes const &indexes,
    std::size_t const recursion_start_idx,
    std::size_t &recursion_end_idx
  ) {
    // Note: a command can't have positionals if it has subcommands, so it suffices to check if first positional token
    // is a subcommand. If it isn't, everything is considered as positionals. Also, the command name is not present in
    // the indexes, so 0 really is the first positional.
    auto const tok_idx = indexes.first_pos_idx_after(recursion_start_idx);
    if (!tok_idx.has_value()) return;
    auto const &tok = tokens[*tok_idx];
    if (!tok.value) return;
    if (auto const cmd_idx = find_cmd(this->cmd_ref.get().subcmds, *tok.value); cmd_idx != -1) {
      int i = 0;
      // clang-format off
      std::apply(
        [this, &i, cmd_idx, &args_map, tokens, &indexes, recursion_end_idx, tok_idx](auto&&... cmd) {
          (void)(( // cast to void to suppress unused warning
          i == cmd_idx
            ? (args_map.submap = CmdParser<typename std::remove_reference_t<decltype(cmd)>::type>(
                cmd.get(), this->extra_info, this->cmd_ref.get().name).get_args_map(tokens, indexes, *tok_idx, recursion_end_idx), true)
            : (++i, false)
          ) || ...);
        },
        this->cmd_ref.get().subcmds
      );
      // clang-format on
      // when coming back from parsing a subcommand, limit the rest of the tokens to up to where we found the subcommand
      recursion_end_idx = *tok_idx - 1;
    }
  }

  template <std::size_t... Is>
  void process_tokens(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokensIndexes const &indexes,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::index_sequence<Is...>
  ) {
    try {
      (this->process_ith_arg<Is>(args_map, tokens, indexes, recursion_start_idx, recursion_end_idx), ...);
      if (!args_map.has_submap()) { // commands can't have both positionals and subcommands
        std::size_t cur_pos_idx = 0;
        // clang-format off
        (this->process_ith_arg<Is>(args_map, tokens, indexes, recursion_start_idx, recursion_end_idx, cur_pos_idx), ...);
        // clang-format on
      }
      (this->check_ith_arg<Is>(args_map), ...);
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), get_cmd_fmt());
    }
  }

  template <std::size_t I>
  void process_ith_arg(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokensIndexes const &indexes,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx
  ) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    switch (arg.type) {
      case ArgType::FLG: {
        std::size_t flg_count = 0;
        if (auto const it = indexes.opts_n_flgs.find(arg.name); it != indexes.opts_n_flgs.end()) {
          for (auto const idx : it->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) flg_count += 1;
          }
        }
        if (auto const it = indexes.opts_n_flgs.find(arg.abbrev); it != indexes.opts_n_flgs.end()) {
          for (auto const idx : it->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) flg_count += 1;
          }
        }

        if (flg_count > 0) consume_arg<I>(args_map, arg, flg_count, this->cmd_ref.get(), this->extra_info);
        break;
      }
      case ArgType::OPT: {
        auto const it_name = indexes.opts_n_flgs.find(arg.name);
        auto const has_name = it_name != indexes.opts_n_flgs.end();
        auto const it_abbrev = indexes.opts_n_flgs.find(arg.abbrev);
        auto const has_abbrev = it_abbrev != indexes.opts_n_flgs.end();
        if (!has_name && !has_abbrev) return;

        auto const reserve_size = (has_name ? it_name->second.size() : 0) + (has_abbrev ? it_abbrev->second.size() : 0);
        std::vector<std::string_view> opt_values; // TODO: make it vector of optionals to support implicit value
        std::vector<std::size_t> all_idxs;
        if (reserve_size > 0) {
          opt_values.reserve(reserve_size);
          all_idxs.reserve(reserve_size);
        }
        if (has_name) {
          // all_idxs.append_range(it_name->second); // TODO(?): use `| filter`
          for (auto const idx : it_name->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) all_idxs.push_back(idx);
          }
        }
        if (has_abbrev) {
          // all_idxs.append_range(it_abbrev->second); // TODO(?): use `| filter`
          for (auto const idx : it_abbrev->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) all_idxs.push_back(idx);
          }
        }
        std::ranges::sort(all_idxs);

        for (auto const idx : all_idxs) {
          if (tokens[idx].value) opt_values.push_back(*tokens[idx].value);
          // the case of `--option value` or `-O value` may have the value as "the next positional"
          // see note in the other process_ith_arg member function
          else if (idx + 1 < tokens.size() && tokens[idx + 1].type == TokenType::IDENTIFIER) {
            opt_values.push_back(*tokens[idx + 1].value);
            this->indexes_used_as_opt_values.insert(idx + 1);
          }
        }

        if (!opt_values.empty())
          consume_arg<I>(args_map, arg, std::cref(opt_values), this->cmd_ref.get(), this->extra_info);
        else throw MissingValue(has_name ? arg.name : arg.abbrev, 1, 0);
        break;
      }
      // ignore positionals in this call
      case ArgType::POS: [[fallthrough]];
      default: break;
    }
  }

  template <std::size_t I>
  void process_ith_arg(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokensIndexes const &indexes,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::size_t &cur_pos_idx
  ) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (arg.type != ArgType::POS) return;
    if (cur_pos_idx >= indexes.positionals.size()) return;
    /* Note: things like `-O value` are scanned as an option followed by an identifier, since the scanner doesn't know
     * if -O is valid or not. So when we encounter that in the process_ith_arg, and it indeed was an option, we save the
     * token index in this->indexes_used_as_opt_values to be ignored here. When we hit such a case, we need to loop until we find the
     * next index that is just an identifier (not one that follows a short valueless option).
     **/
    auto tok_idx = indexes.positionals[cur_pos_idx];
    while (this->indexes_used_as_opt_values.contains(tok_idx) || tok_idx <= recursion_start_idx) {
      if (cur_pos_idx + 1 >= indexes.positionals.size() || tok_idx > recursion_end_idx) return;
      tok_idx = indexes.positionals[++cur_pos_idx];
    }
    if (tok_idx >= tokens.size()) return;

    cur_pos_idx += 1;
    auto const &tok = tokens[tok_idx];
    if (tok.value) consume_arg<I>(args_map, arg, *tok.value, this->cmd_ref.get(), this->extra_info);
  }

  template <std::size_t I>
  void check_ith_arg(ArgsMap<Cmd const> &args_map) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (!args_map.template has_value<I>()) {
      if (arg.is_required) throw MissingRequiredArgument(this->cmd_ref.get().name, arg.name, get_cmd_fmt());
      if (arg.has_default()) std::get<I>(args_map.args) = *arg.default_value;
    }
  }
};

} // namespace opz

#endif // OPZIONI_PARSING_HPP
