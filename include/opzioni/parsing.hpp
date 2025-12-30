#ifndef OPZIONI_PARSING_HPP
#define OPZIONI_PARSING_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
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

template <concepts::Cmd... Cmds>
[[nodiscard]] constexpr int
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

  [[nodiscard]] ArgsMap<Cmd const> operator()(std::span<char const *> const args) {
    auto scanner = Scanner(args);
    auto const tokens = scanner();
    auto const indices = index_tokens(tokens);
    auto map = this->get_args_map(args, tokens, indices, 0, tokens.size() - 1);
    return map;
  }

  [[nodiscard]] ArgsMap<Cmd const> operator()(int const argc, char const *argv[]) {
    auto const args = std::span{argv, static_cast<std::size_t>(argc)};
    return (*this)(args);
  }

private:

  template <concepts::Cmd>
  friend class CmdParser;

  std::set<std::size_t> indices_used_as_opt_values;
  std::map<std::uint_least32_t, std::size_t> parsed_arg_idx_for_group;

  CmdParser(Cmd const &cmd, ExtraInfo const &extra_info, std::string_view const parent_cmd_name) : cmd_ref(cmd) {
    this->extra_info.parent_cmds_names.reserve(extra_info.parent_cmds_names.size() + 1);
    for (auto const name : extra_info.parent_cmds_names) {
      this->extra_info.parent_cmds_names.push_back(name);
    }
    this->extra_info.parent_cmds_names.push_back(parent_cmd_name);
  }

  [[nodiscard]] auto get_cmd_fmt() const noexcept { return CmdFmt(this->cmd_ref.get(), this->extra_info); }

  [[nodiscard]] auto get_args_map(
    std::span<char const *> const args,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx,
    std::size_t recursion_end_idx
  ) {
    auto args_map = ArgsMap<Cmd const>();
    args_map.exec_path = *tokens[recursion_start_idx].value;
    // TODO: checking for subcommand before checking for options break the `--opt val` format for ill-formed params
    if constexpr (std::tuple_size_v<decltype(this->cmd_ref.get().subcmds)> > 0) {
      this->parse_possible_subcmd(args, args_map, tokens, indices, recursion_start_idx, recursion_end_idx);
    }
    // further args have to be > recursion_start_idx and <= recursion_end_idx
    std::set<std::size_t> consumed_indices;
    consumed_indices.insert(recursion_start_idx);
    constexpr auto args_size = std::tuple_size_v<decltype(this->cmd_ref.get().args)>;
    this->process_tokens(
      args_map,
      tokens,
      indices,
      recursion_start_idx,
      recursion_end_idx,
      consumed_indices,
      std::make_index_sequence<args_size>()
    );
    this->check_unknown_args(args, tokens, recursion_start_idx, recursion_end_idx, consumed_indices);
    return args_map;
  }

  void parse_possible_subcmd(
    std::span<char const *> const args,
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx,
    std::size_t &recursion_end_idx
  ) {
    // Note: a command can't have positionals if it has subcommands, so it suffices to check if first positional token
    // is a subcommand. If it's not, then the user provided an unknown subcommand, since this method is only called
    // if the command has any subcommand. Also, the command name is not present in the indices,
    // so 0 (or recursion_start_idx) really is the first positional.
    auto const tok_idx = indices.first_pos_idx_after(recursion_start_idx);
    if (!tok_idx.has_value()) return;
    auto const &tok = tokens[*tok_idx];
    if (!tok.value) return;
    if (auto const cmd_idx = find_cmd(this->cmd_ref.get().subcmds, *tok.value); cmd_idx != -1) {
      int i = 0;
      // clang-format off
      std::apply(
        [this, &i, cmd_idx, &args_map, &args, tokens, &indices, tok_idx, recursion_end_idx](auto&&... cmd) {
          (void)(( // cast to void to suppress unused warning
          i == cmd_idx
            ? (args_map.submap = CmdParser<typename std::remove_reference_t<decltype(cmd)>::type>(
                cmd.get(), this->extra_info, this->cmd_ref.get().name).get_args_map(args, tokens, indices, *tok_idx, recursion_end_idx), true)
            : (++i, false)
          ) || ...);
        },
        this->cmd_ref.get().subcmds
      );
      // clang-format on
      // when coming back from parsing a subcommand, limit the rest of the tokens to up to the token before the subcmd
      recursion_end_idx = *tok_idx - 1;
    } else {
      throw UnknownSubcommand(this->cmd_ref.get().name, *tok.value, this->get_cmd_fmt());
    }
  }

  template <std::size_t... Is>
  void process_tokens(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::set<std::size_t> &consumed_indices,
    std::index_sequence<Is...>
  ) {
    try {
      // clang-format off
      (this->process_ith_flg_or_opt<Is>(args_map, tokens, indices, recursion_start_idx, recursion_end_idx, consumed_indices), ...);
      // only try and process positionals if there are no subcommands
      // because commands can't have both them and positionals
      if constexpr (std::tuple_size_v<decltype(this->cmd_ref.get().subcmds)> == 0) {
        (this->process_ith_pos<Is>(args_map, tokens, indices, recursion_start_idx, recursion_end_idx, consumed_indices), ...);
      }
      // clang-format on
      (this->post_process_ith_arg<Is>(args_map, tokens, indices, recursion_start_idx), ...);
      (this->check_missing_ith_arg<Is>(args_map), ...);
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), get_cmd_fmt());
    }
  }

  template <std::size_t I>
  void process_ith_flg_or_opt(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::set<std::size_t> &consumed_indices
  ) {
    switch (auto const &arg = std::get<I>(this->cmd_ref.get().args); arg.kind) {
      case ArgKind::FLG: {
        std::size_t flg_count = 0;
        if (auto const it = indices.opts_n_flgs.find(arg.name); it != indices.opts_n_flgs.end()) {
          for (auto const idx : it->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) {
              flg_count += 1;
              consumed_indices.insert(idx);
            }
          }
        }
        if (auto const it = indices.opts_n_flgs.find(arg.abbrev); it != indices.opts_n_flgs.end()) {
          for (auto const idx : it->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) {
              flg_count += 1;
              consumed_indices.insert(idx);
            }
          }
        }

        if (flg_count > 0) consume_arg<I>(args_map, arg, flg_count, this->cmd_ref.get(), this->extra_info);
        break;
      }
      case ArgKind::OPT: {
        auto const it_name = indices.opts_n_flgs.find(arg.name);
        auto const has_name = it_name != indices.opts_n_flgs.end();
        auto const it_abbrev = indices.opts_n_flgs.find(arg.abbrev);
        auto const has_abbrev = it_abbrev != indices.opts_n_flgs.end();
        if (!has_name && !has_abbrev) return;

        auto const reserve_size = (has_name ? it_name->second.size() : 0) + (has_abbrev ? it_abbrev->second.size() : 0);
        std::vector<std::string_view> opt_values; // TODO: make it vector of optionals to support implicit value
        std::vector<std::size_t> all_idxs;
        if (reserve_size > 0) {
          opt_values.reserve(reserve_size);
          all_idxs.reserve(reserve_size);
        }
        if (has_name) {
          for (auto const idx : it_name->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) {
              all_idxs.push_back(idx);
              consumed_indices.insert(idx);
            }
          }
        }
        if (has_abbrev) {
          for (auto const idx : it_abbrev->second) {
            if (idx > recursion_start_idx && idx <= recursion_end_idx) {
              all_idxs.push_back(idx);
              consumed_indices.insert(idx);
            }
          }
        }
        std::ranges::sort(all_idxs);

        for (auto const idx : all_idxs) {
          if (tokens[idx].value) opt_values.push_back(*tokens[idx].value);
          // the case of `--option value` or `-O value` may have the value as "the next positional"
          // see note in the other process_ith_arg member function
          else if (idx + 1 < tokens.size() && tokens[idx + 1].kind == TokenKind::IDENTIFIER) {
            opt_values.push_back(*tokens[idx + 1].value);
            this->indices_used_as_opt_values.insert(idx + 1);
            consumed_indices.insert(idx + 1);
          }
        }

        if (!opt_values.empty())
          consume_arg<I>(args_map, arg, std::cref(opt_values), this->cmd_ref.get(), this->extra_info);
        else throw MissingValue(has_name ? arg.name : arg.abbrev, 1, 0);
        break;
      }
      // ignore positionals in this call
      case ArgKind::POS: [[fallthrough]];
      default: break;
    }
  }

  template <std::size_t I>
  void process_ith_pos(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::set<std::size_t> &consumed_indices
  ) {
    static std::size_t cur_pos_idx = 0;
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (arg.kind != ArgKind::POS) return;
    if (cur_pos_idx >= indices.positionals.size()) return;
    /* Note: things like `-O value` are scanned as an option followed by an identifier, since the scanner doesn't know
     * if -O is valid or not. So when we encounter that in the process_ith_arg, and it indeed was an option, we save the
     * token index in this->indices_used_as_opt_values to be ignored here. When we hit such a case, we need to loop
     * until we find the next index that is just an identifier (not one that follows a short valueless option).
     **/
    auto tok_idx = indices.positionals[cur_pos_idx];
    while (this->indices_used_as_opt_values.contains(tok_idx) || tok_idx <= recursion_start_idx) {
      if (cur_pos_idx + 1 >= indices.positionals.size() || tok_idx > recursion_end_idx) return;
      tok_idx = indices.positionals[++cur_pos_idx];
    }
    if (tok_idx >= tokens.size()) return;

    cur_pos_idx += 1;
    consumed_indices.insert(tok_idx);
    if (auto const &tok = tokens[tok_idx]; tok.value)
      consume_arg<I>(args_map, arg, *tok.value, this->cmd_ref.get(), this->extra_info);
  }

  template <std::size_t I>
  void post_process_ith_arg(
    ArgsMap<Cmd const> &args_map,
    std::span<Token const> const tokens,
    TokenIndices const &indices,
    std::size_t const recursion_start_idx
  ) {
    static std::size_t cur_pos_idx = 0;
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (args_map.template has_value<I>()) {
      // get index of arg in argv (we know it exists because it's present in args_map)
      auto const arg_idx = [&arg, &indices, recursion_start_idx] {
        if (arg.kind == ArgKind::POS) return *indices.nth_pos_idx_after(recursion_start_idx, cur_pos_idx);
        if (auto const it_name = indices.opts_n_flgs.find(arg.name); it_name != indices.opts_n_flgs.end())
          return it_name->second[0];
        auto const it_abbrev = indices.opts_n_flgs.find(arg.abbrev);
        return it_abbrev->second[0];
      }();

      // check if we already have parsed an argument of the same group
      if (arg.grp_kind == GroupKind::MUTUALLY_EXCLUSIVE) {
        if (auto const grp_it = this->parsed_arg_idx_for_group.find(arg.grp_id);
            grp_it != this->parsed_arg_idx_for_group.end()) {
          auto const tok_current =
            std::find_if(tokens.begin(), tokens.end(), [arg_idx](auto const &t) { return t.args_idx == arg_idx; });
          auto const tok_previous = std::find_if(tokens.begin(), tokens.end(), [&grp_it](auto const &t) {
            return t.args_idx == grp_it->second;
          });
          throw ConflictingArguments(
            this->cmd_ref.get().name, tok_current->get_id(), tok_previous->get_id(), this->get_cmd_fmt()
          );
        }
      }

      // if not, register we now have
      if (arg.has_group()) {
        this->parsed_arg_idx_for_group[arg.grp_id] = arg_idx;
      }
    }
    cur_pos_idx += static_cast<std::size_t>(arg.kind == ArgKind::POS);
  }

  template <std::size_t I>
  void check_missing_ith_arg(ArgsMap<Cmd const> &args_map) const {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (!args_map.template has_value<I>()) {
      if (arg.is_required) throw MissingRequiredArgument(this->cmd_ref.get().name, arg.name, get_cmd_fmt());
      if (arg.has_default()) std::get<I>(args_map.args) = *arg.default_value;
      if (arg.grp_kind == GroupKind::ALL_REQUIRED && this->parsed_arg_idx_for_group.contains(arg.grp_id))
        throw MissingGroupedArguments(this->cmd_ref.get().name, arg.name, get_cmd_fmt());
    }
  }

  void check_unknown_args(
    std::span<char const *> const args,
    std::span<Token const> const tokens,
    std::size_t const recursion_start_idx,
    std::size_t const recursion_end_idx,
    std::set<std::size_t> const &consumed_indices
  ) const {
    if (std::size_t const recursion_amount_indices = recursion_end_idx - recursion_start_idx + 1;
        consumed_indices.size() < recursion_amount_indices) {
      std::vector<std::string_view> unknown_args;
      for (std::size_t idx = recursion_start_idx; idx <= recursion_end_idx; ++idx) {
        if (!consumed_indices.contains(idx)) unknown_args.emplace_back(args[tokens[idx].args_idx]);
      }
      throw UnknownArguments(this->cmd_ref.get().name, unknown_args, this->get_cmd_fmt());
    }
  }
};

} // namespace opz

#endif // OPZIONI_PARSING_HPP
