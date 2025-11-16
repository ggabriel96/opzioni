#ifndef OPZIONI_PARSING_HPP
#define OPZIONI_PARSING_HPP

#include <cstddef>
#include <functional>
#include <set>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

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
  constexpr static ArgView from(std::size_t tuple_idx, Arg<T, Tag> const &other) {
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
  // TODO: use something like the frozen library instead of this
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

struct ArgsIndexes {
  std::vector<std::size_t> positionals;
  std::map<std::string_view, std::vector<std::size_t>> opts_n_flgs;
};

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

  ArgsMap<Cmd const> operator()(std::span<char const *> args) {
    auto scanner = Scanner(args);
    auto const tokens = scanner();
    auto const args_indexes = this->get_args_indexes(tokens);
    // TODO: check we did not receive unknown arguments (throw UnknownArgument(this->cmd_ref.get().name, flag, get_cmd_fmt());)
    auto map = this->get_args_map(tokens, args_indexes);
    return map;
  }

  ArgsMap<Cmd const> operator()(int argc, char const *argv[]) {
    auto const args = std::span{argv, static_cast<std::size_t>(argc)};
    return (*this)(args);
  }

private:
  template <concepts::Cmd>
  friend class CmdParser;

  std::set<std::size_t> ignore_idxs;

  CmdParser(Cmd const &cmd, ExtraInfo extra_info, std::string_view parent_cmd_name) : cmd_ref(cmd) {
    this->extra_info.parent_cmds_names.reserve(extra_info.parent_cmds_names.size() + 1);
    for (auto const name : extra_info.parent_cmds_names) {
      this->extra_info.parent_cmds_names.push_back(name);
    }
    this->extra_info.parent_cmds_names.push_back(parent_cmd_name);
  }

  auto get_cmd_fmt() const noexcept { return CmdFmt(this->cmd_ref.get(), this->extra_info); }

  auto get_args_indexes(std::vector<Token> const &tokens) {
    auto indexes = ArgsIndexes();
    if (!tokens.empty()) { // should never happen
      for (std::size_t index = 1; index < tokens.size(); ++index) {
        auto const &tok = tokens[index];
        switch (tok.type) {
          case TokenType::PROG_NAME:
            break;
          case TokenType::DASH_DASH:
            // +1 to ignore the dash-dash
            for (std::size_t offset = index + 1; offset < tokens.size(); ++offset) {
              indexes.positionals.push_back(offset);
            }
            index = tokens.size(); // break below refers to the switch, not the for loop
            break;
          case TokenType::FLG: [[fallthrough]];
          case TokenType::OPT_OR_FLG_LONG: [[fallthrough]];
          case TokenType::OPT_LONG_AND_VALUE: [[fallthrough]];
          case TokenType::OPT_SHORT_AND_VALUE:
            indexes.opts_n_flgs[*tok.name].push_back(index);
            break;
          case TokenType::IDENTIFIER:
            indexes.positionals.push_back(index);
            break;
        }
      }
    }
    return indexes;
  }

  auto get_args_map(std::vector<Token> const &tokens, ArgsIndexes const &indexes) {
    constexpr auto args_size = std::tuple_size_v<decltype(this->cmd_ref.get().args)>;
    return process_tokens(tokens, indexes, this->extra_info, std::make_index_sequence<args_size>());
  }

  template <std::size_t... Is>
  auto process_tokens(std::vector<Token> const &tokens, ArgsIndexes const &indexes, ExtraInfo extra_info, std::index_sequence<Is...>) {
    auto args_map = ArgsMap<Cmd const>();
    args_map.exec_path = *tokens[0].value;
    std::size_t cur_pos_idx = 0;
    try {
      (process_ith_arg<Is>(args_map, tokens, indexes, extra_info), ...);
      (process_ith_arg<Is>(args_map, tokens, indexes, extra_info, cur_pos_idx), ...);
      (check_ith_arg<Is>(args_map), ...);
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), get_cmd_fmt());
    }
    return args_map;
  }

  template <std::size_t I>
  void process_ith_arg(ArgsMap<Cmd const> &args_map, std::vector<Token> const &tokens, ArgsIndexes const &indexes, ExtraInfo extra_info) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    switch (arg.type) {
      case ArgType::FLG: {
        std::size_t flg_count = 0;
        if (auto const it = indexes.opts_n_flgs.find(arg.name); it != indexes.opts_n_flgs.end())
          flg_count += it->second.size();
        if (auto const it = indexes.opts_n_flgs.find(arg.abbrev); it != indexes.opts_n_flgs.end())
          flg_count += it->second.size();

        if (flg_count > 0) consume_arg<I>(args_map, arg, flg_count, this->cmd_ref.get(), extra_info);
        break;
      }
      case ArgType::OPT: {
        auto const it_name = indexes.opts_n_flgs.find(arg.name);
        auto const has_name = it_name != indexes.opts_n_flgs.end();
        auto const it_abbrev = indexes.opts_n_flgs.find(arg.abbrev);
        auto const has_abbrev = it_abbrev != indexes.opts_n_flgs.end();
        auto const reserve_size = (has_name ? it_name->second.size() : 0) + (has_abbrev ? it_abbrev->second.size() : 0);
        std::vector<std::string_view> opt_values;
        std::vector<std::size_t> all_idxs;
        if (reserve_size > 0) {
          opt_values.reserve(reserve_size);
          all_idxs.reserve(reserve_size);
        }
        if (has_name) all_idxs.append_range(it_name->second);
        if (has_abbrev) all_idxs.append_range(it_abbrev->second);
        std::ranges::sort(all_idxs);

        for (auto const idx : all_idxs) {
          if (tokens[idx].value) opt_values.push_back(*tokens[idx].value);
          // the case of `--option value` or `-O value` may have the value as "the next positional"
          // see note in the other process_ith_arg member function
          else if (idx + 1 < tokens.size() && tokens[idx + 1].type == TokenType::IDENTIFIER) {
            opt_values.push_back(*tokens[idx + 1].value);
            this->ignore_idxs.insert(idx + 1);
          }
        }

        consume_arg<I>(args_map, arg, std::cref(opt_values), this->cmd_ref.get(), extra_info);
        break;
      }
      // ignore positionals in this call
      case ArgType::POS: [[fallthrough]];
      default:
        break;
    }
  }

  template <std::size_t I>
  void process_ith_arg(ArgsMap<Cmd const> &args_map, std::vector<Token> const &tokens, ArgsIndexes const &indexes, ExtraInfo extra_info, std::size_t &cur_pos_idx) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (arg.type != ArgType::POS) return;
    // if (auto const idx = find_cmd(this->cmd_ref.get().subcmds, *tok.value); idx != -1) {
    //   auto const rest = args.subspan(index);
    //   int i = 0;
    //   // clang-format off
    //   std::apply(
    //     [this, &i, idx, &map, rest](auto&&... cmd) {
    //       (void)(( // cast to void to suppress unused warning
    //       i == idx
    //         ? (map.submap = CmdParser<typename std::remove_reference_t<decltype(cmd)>::type>(
    //             cmd.get(), this->extra_info, this->cmd_ref.get().name).get_strings_map(rest), true)
    //         : (++i, false)
    //       ) || ...);
    //     },
    //     this->cmd_ref.get().subcmds
    //   );
    //   // clang-format on
    //   index += rest.size();
    // }

    if (cur_pos_idx >= indexes.positionals.size()) return;    
    /* Note: things like `-O value` are scanned as an option followed by an identifier, since the scanner doesn't know if -O is valid or not.
     * So when we encounter that in the process_ith_arg and it indeed was an option, we save the token index in this->ignore_idxs to be ignored here.
     * When we hit such a case, we need to loop until we find the next index that is just an identifier (not one that follows a short valueless option).
     **/
    auto tok_idx = indexes.positionals[cur_pos_idx];
    while (this->ignore_idxs.contains(tok_idx)) {
      if (cur_pos_idx + 1 >= indexes.positionals.size()) return;
      else tok_idx = indexes.positionals[++cur_pos_idx];
    }

    auto const &tok = tokens[tok_idx];
    if (tok.value) consume_arg<I>(args_map, arg, *tok.value, this->cmd_ref.get(), extra_info);
    cur_pos_idx += 1;
  }

  template <std::size_t I>
  void check_ith_arg(ArgsMap<Cmd const> &args_map) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    if (!args_map.template has_value<I>()) {
      if (arg.is_required)
        throw MissingRequiredArgument(this->cmd_ref.get().name, arg.name, get_cmd_fmt());
      if (arg.has_default())
        std::get<I>(args_map.args) = *arg.default_value;
    }
  }
};

} // namespace opz

#endif // OPZIONI_PARSING_HPP
