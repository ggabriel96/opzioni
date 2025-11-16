#ifndef OPZIONI_PARSING_HPP
#define OPZIONI_PARSING_HPP

#include <cstddef>
#include <functional>
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
    auto const strings_map = this->get_strings_map(args, tokens);
    // TODO: check we did not receive unknown arguments (throw UnknownArgument(this->cmd_ref.get().name, flag, get_cmd_fmt());)
    auto map = this->get_args_map(strings_map);
    return map;
  }

  ArgsMap<Cmd const> operator()(int argc, char const *argv[]) {
    auto const args = std::span{argv, static_cast<std::size_t>(argc)};
    return (*this)(args);
  }

private:
  template <concepts::Cmd>
  friend class CmdParser;

  CmdParser(Cmd const &cmd, ExtraInfo extra_info, std::string_view parent_cmd_name) : cmd_ref(cmd) {
    this->extra_info.parent_cmds_names.reserve(extra_info.parent_cmds_names.size() + 1);
    for (auto const name : extra_info.parent_cmds_names) {
      this->extra_info.parent_cmds_names.push_back(name);
    }
    this->extra_info.parent_cmds_names.push_back(parent_cmd_name);
  }

  auto get_cmd_fmt() const noexcept { return CmdFmt(this->cmd_ref.get(), this->extra_info); }

  auto get_strings_map(std::span<char const *> args, std::vector<Token> const &tokens) {
    auto map = StringsMap();
    if (!tokens.empty()) { // should never happen
      for (std::size_t index = 0; index < tokens.size(); ++index) {
        auto const &tok = tokens[index];
        switch (tok.type) {
          case TokenType::PROG_NAME:
            map.exec_path = *tok.value;
            break;
          case TokenType::DASH: // TODO: dash is a regular identifier?
            break;
          case TokenType::DASH_DASH:
            // +1 to ignore the dash-dash
            for (std::size_t offset = index + 1; offset < tokens.size(); ++offset) {
              map.positionals.push_back(args[offset]);
            }
            index = tokens.size(); // break below refers to the switch, not the for loop
            break;
          case TokenType::FLG:
            map.flags[*tok.name] += 1;
            break;
          case TokenType::OPT_OR_FLG_LONG:
            // the case of `--option value` may have the value as "the next positional"
            if (index + 1 < tokens.size() && tokens[index + 1].type == TokenType::IDENTIFIER) {
              // consider the next token as both possible option value and possible positional below (don't skip next index)
              map.options[*tok.name].emplace_back(*tokens[index + 1].value);
            } else {
              map.flags[*tok.name] += 1;
            }
            break;
          case TokenType::OPT_LONG_AND_VALUE: [[fallthrough]];
          case TokenType::OPT_SHORT_AND_VALUE:
            map.options[*tok.name].emplace_back(*tok.value);
            break;
          case TokenType::IDENTIFIER:
            map.positionals.push_back(*tok.value);
            break;
        }
      }
    }
    return map;
  }

  auto get_args_map(StringsMap const &strings_map) {
    constexpr auto args_size = std::tuple_size_v<decltype(this->cmd_ref.get().args)>;
    return process_strings_map(strings_map, this->extra_info, std::make_index_sequence<args_size>());
  }

  template <std::size_t... Is>
  auto process_strings_map(StringsMap const &strings_map, ExtraInfo extra_info, std::index_sequence<Is...>) {
    auto args_map = ArgsMap<Cmd const>();
    std::size_t cur_pos_idx = 0;
    try {
      (process_ith_arg<Is>(args_map, strings_map, extra_info, cur_pos_idx), ...);
    } catch (std::runtime_error const &e) {
      throw UserError(e.what(), get_cmd_fmt());
    }
    return args_map;
  }

  template <std::size_t I>
  void process_ith_arg(ArgsMap<Cmd const> &args_map, StringsMap const &strings_map, ExtraInfo extra_info, std::size_t &cur_pos_idx) {
    auto const &arg = std::get<I>(this->cmd_ref.get().args);
    switch (arg.type) {
      case ArgType::FLG: {
        std::size_t flg_count = 0;
        if (auto const it = strings_map.flags.find(arg.name); it != strings_map.flags.end())
          flg_count += it->second;
        else if (auto const it = strings_map.flags.find(arg.abbrev); it != strings_map.flags.end())
          flg_count += it->second;

        // also search in options because every long option followed by a positional is considered
        // as an option followed by its value, but it may be a flag followed by a positional
        if (auto const it = strings_map.options.find(arg.name); it != strings_map.options.end())
          flg_count += it->second.size();
        else if (auto const it = strings_map.options.find(arg.abbrev); it != strings_map.options.end())
          flg_count += it->second.size();

        if (flg_count > 0) consume_arg<I>(args_map, arg, flg_count, this->cmd_ref.get(), extra_info);
        break;
      }
      case ArgType::OPT: {
        auto it = strings_map.options.find(arg.name);
        if (it == strings_map.options.end()) it = strings_map.options.find(arg.abbrev);
        if (it != strings_map.options.end()) {
          auto const &arg_value = it->second;
          consume_arg<I>(args_map, arg, std::cref(arg_value), this->cmd_ref.get(), extra_info);
        }
        break;
      }
      case ArgType::POS: [[fallthrough]];
      default: {
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
        if (cur_pos_idx >= strings_map.positionals.size())
          throw UnexpectedPositional(this->cmd_ref.get().name, arg.name, this->cmd_ref.get().amount_pos, get_cmd_fmt());
        auto const &arg_value = strings_map.positionals.at(cur_pos_idx);
        consume_arg<I>(args_map, arg, arg_value, this->cmd_ref.get(), extra_info);
        cur_pos_idx += 1;
        break;
      }
    }
    if (!args_map.template has_value<I>()) {
      if (arg.is_required)
        throw MissingRequiredArgument(this->cmd_ref.get().name, arg.name, get_cmd_fmt());
      if (arg.has_default())
        std::get<I>(args_map.t_args) = *arg.default_value;
    }
  }
};

} // namespace opz

#endif // OPZIONI_PARSING_HPP
