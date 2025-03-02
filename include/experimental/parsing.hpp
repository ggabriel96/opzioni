#ifndef OPZIONI_PARSING_H
#define OPZIONI_PARSING_H

#include <algorithm>
#include <any>
#include <cctype>
#include <concepts>
#include <map>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "converters.hpp"
#include "exceptions.hpp"
#include "experimental/fixed_string.hpp"
#include "experimental/program.hpp"
#include "experimental/string_list.hpp"
#include "experimental/type_list.hpp"

namespace opz {

// +---------------------+
// |      fwd decls      |
// +---------------------+

struct ParsedOption {
  ArgView arg;
  // no string and empty string mean different things here
  std::optional<std::string_view> value;
};

constexpr bool is_dash_dash(std::string_view const) noexcept;
constexpr bool looks_positional(std::string_view const) noexcept;
constexpr std::string_view get_if_short_flags(std::string_view const) noexcept;
constexpr std::string_view get_if_long_flag(std::string_view const) noexcept;
constexpr ParsedOption try_parse_option(std::string_view const) noexcept;

template <typename... Ts>
auto FindArg(std::tuple<Arg<Ts>...> haystack, std::predicate<ArgView> auto p) {
  return std::apply(
    [&p](auto&&... elem) {
      std::optional<ArgView> ret = std::nullopt;

      (void) // cast to void to suppress unused warning
      (
        (p(elem) ? (ret = elem, true) : (false)) || ...
      );

      return ret;
    },
    haystack
  );
}

// +-----------------------+
// |   main parsing code   |
// +-----------------------+

struct ArgsView {
  std::string_view exec_path{};
  // TODO: put positionals and options on the same map when we
  // start querying the program args?
  std::vector<std::string_view> positionals;
  std::map<std::string_view, std::string_view> options;

  void print_debug() const noexcept;
};

template <typename...>
struct ArgsMap;

template <fixed_string... Names, typename... Types>
struct ArgsMap<StringList<Names...>, TypeList<Types...>> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;

  template<fixed_string Name>
  typename GetType<Name, arg_names, arg_types>::type get() const {
    using T = GetType<Name, arg_names, arg_types>::type;
    static_assert(!std::is_same_v< T, void >, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw opzioni::ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  // TODO: get_or(<default value>)

  bool has(std::string_view name) const noexcept { return args.contains(name); }

  auto size() const noexcept { return args.size(); }
};

template <typename...>
struct ArgParser;

template <fixed_string... Names, typename... Types>
struct ArgParser<StringList<Names...>, TypeList<Types...>> {
  using arg_names = StringList<Names...>;
  using arg_types = TypeList<Types...>;

  Program<arg_names, arg_types> const &program;

  ArgParser(Program<arg_names, arg_types> const &program) : program(program) {}

  // auto operator()(std::span<char const *> args) {
  //   ArgsMap map;
  //   map.exec_path = args[0];
  //   return map;
  // }

  auto get_args_view(std::span<char const *> args) {
    ArgsView view;
    if (args.size() > 0) {
      view.exec_path = std::string_view(args[0]);
      std::size_t current_positional_idx = 0;
      for (std::size_t index = 1; index < args.size();) {
        auto const arg = std::string_view(args[index]);
        if (is_dash_dash(arg)) {
          // +1 to ignore the dash-dash
          for (std::size_t offset = index + 1; offset < args.size();) {
            offset += assign_positional(view, args.subspan(offset), current_positional_idx);
            ++current_positional_idx;
          }
          index = args.size();
          // } else if (auto cmd = is_command(program, arg); cmd != nullptr) {
          //   index += assign_command(map, args.subspan(index), *cmd);
        } else if (looks_positional(arg)) {
          index += assign_positional(view, args.subspan(index), current_positional_idx);
          ++current_positional_idx;
        } else if (auto const option = try_parse_option(arg); option.has_value()) {
          index += assign_option(view, *option, args.subspan(index));
        } else if (auto const flag = get_if_long_flag(arg); !flag.empty()) {
          index += assign_long_flag(view, flag);
        } else if (auto const flags = get_if_short_flags(arg); !flags.empty()) {
          index += assign_short_flags(view, flags);
        } else {
          throw opzioni::UnknownArgument(arg);
        }
      }
    }
    return view;
  }

  std::size_t assign_positional(ArgsView &view, std::span<char const *> args, std::size_t cur_pos_idx) const {
    if (cur_pos_idx + 1 > program.amount_pos)
      throw opzioni::UnexpectedPositional(args[0], program.amount_pos);
    view.positionals.emplace_back(args[0]);
    return 1;
  }

  std::optional<ParsedOption> try_parse_option(std::string_view const whole_arg) noexcept {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    bool const has_equals = eq_idx != std::string_view::npos;
    if (num_of_dashes == 1) {
      // short option, e.g. `-O`
      auto const name = whole_arg.substr(1, 1);
      auto const it = FindArg(program.args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.abbrev; });
      if (!it) return std::nullopt;

      if (has_equals) {
        if (whole_arg.length() > 3) {
          // has equals followed by some value, e.g. `-O=2`
          return ParsedOption{.arg = *it, .value = whole_arg.substr(3)};
        }
        // TODO: should this `-O=` be handled like this?
        // return {name, ""};
        return std::nullopt; // tmply considered not an option
      }

      if (whole_arg.length() > 2 && std::isupper(name[0])) {
        // only followed by some value, e.g. `-O2`
        return ParsedOption{.arg = *it, .value = whole_arg.substr(2)};
      }

      // case left: has no value (next CLI argument could be it)
      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    if (num_of_dashes == 2 && whole_arg.length() > 3) {
      // long option, e.g. `--name`

      if (has_equals) {
        auto const name = whole_arg.substr(2, eq_idx - 2);
        auto const it = FindArg(program.args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
        if (!it) return std::nullopt;

        auto const value = whole_arg.substr(eq_idx + 1);
        return ParsedOption{.arg = *it, .value = value};
      }

      // has no value (long options cannot have "glued" values like `-O2`; next CLI argument could be it)
      auto const name = whole_arg.substr(2);
      auto const it = FindArg(program.args, [name](auto const &a) { return a.type == ArgType::OPT && name == a.name; });
      if (!it) return std::nullopt;
      return ParsedOption{.arg = *it, .value = std::nullopt};
    }

    // not an option
    return std::nullopt;
  }

  std::size_t assign_option(ArgsView &view, ParsedOption const option, std::span<char const *> args) const {
    if (option.value) {
      // value lookup is by name, not abbrev
      view.options[option.arg.name] = *option.value;
      return 1;
    }

    if (args.size() > 1 && looks_positional(args[1])) {
      // value lookup is by name, not abbrev
      view.options[option.arg.name] = args[1];
      return 2;
    }

    // report error using the name/abbrev the user used
    throw opzioni::MissingValue(option.arg.name, 1, 0);
  }

  std::size_t assign_long_flag(ArgsView &view, std::string_view const flag) const {
    auto const it = FindArg(program.args, [&flag](auto const &a) { return a.name == flag; });
    if (!it) {
      throw opzioni::UnknownArgument(flag);
    }
    if (it->type != ArgType::FLG) {
      throw opzioni::WrongType(flag, ToString(it->type), ToString(ArgType::FLG));
    }

    view.options[it->name] = "";
    return 1;
  }

  std::size_t assign_short_flags(ArgsView &view, std::string_view const flags) const {
    for (std::size_t i = 0; i < flags.size(); ++i) {
      auto const flag = flags.substr(i, 1);
      auto const it = FindArg(program.args, [&flag](auto const &a) { return a.abbrev == flag; });
      if (!it) {
        throw opzioni::UnknownArgument(flag);
      }
      if (it->type != ArgType::FLG) {
        throw opzioni::WrongType(flag, ToString(it->type), ToString(ArgType::FLG));
      }

      // value lookup is by name, not abbrev
      view.options[it->name] = "";
    }
    return 1;
  }

  auto get_args_map(ArgsView const &view) const {
    auto map = ArgsMap<arg_names, arg_types>{.exec_path = view.exec_path};
    std::size_t pos_count = 0;

    std::apply(
      [this, &map, &view, &pos_count](auto&&... arg) {
        (this->process(arg, map, view, pos_count), ...);
      },
      program.args
    );

    return map;
  }

  template<typename T>
  auto process(Arg<T> const &arg, ArgsMap<arg_names, arg_types> &map, ArgsView const &view, std::size_t &pos_count) const {
    switch (arg.type) {
      case ArgType::POS: {
        std::print("process POS {}, pos_count {}\n", arg.name, pos_count);
        if (pos_count < view.positionals.size()) {
          map.args[arg.name] = opzioni::convert<T>(view.positionals[pos_count]);
          pos_count += 1;
        } else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        // check for arg being required is done in a later step
        break;
      }
      case ArgType::OPT: {
        std::print("process OPT {}\n", arg.name);
        auto const opt = view.options.find(arg.name);
        if (opt != view.options.end()) {
          std::print("OPT {} found, value: {}\n", arg.name, opt->second);
          map.args[arg.name] = opzioni::convert<T>(opt->second);
        } else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        // check for arg being required is done in a later step
        break;
      }
      case ArgType::FLG: {
        std::print("process FLG {}\n", arg.name);
        if (view.options.contains(arg.name)) map.args[arg.name] = true;
        else if (arg.has_default()) map.args[arg.name] = *arg.default_value;
        break;
      }
    }
  }

  void check_contains_required(ArgsMap<arg_names, arg_types> const &map) const {
    std::vector<std::string_view> missing_arg_names;
    std::apply(
      [&map, &missing_arg_names](auto&&... arg) {
        (void) // cast to void to suppress unused warning
        (
          (!map.has(arg.name) && arg.is_required ? missing_arg_names.push_back(arg.name) : (void)0), ...
        );
      },
      program.args
    );

    if (!missing_arg_names.empty())
      throw opzioni::MissingRequiredArguments(missing_arg_names);
  }
};

template <fixed_string... Names, typename... Types>
ArgParser(Program<StringList<Names...>, TypeList<Types...>> const &program) -> ArgParser<StringList<Names...>, TypeList<Types...>>;

template <fixed_string... Names, typename... Types>
auto parse(Program<StringList<Names...>, TypeList<Types...>> const &program, int argc, char const *argv[]) {
  auto const args = std::span<char const *>{argv, static_cast<std::size_t>(argc)};

  auto parser  = ArgParser(program);
  auto const view = parser.get_args_view(args);
  view.print_debug();

  auto const map = parser.get_args_map(view);
  // done separately from `get_args_map` in order to report all missing required args
  parser.check_contains_required(map);

  return map;
}

// +---------------------+
// |   parsing helpers   |
// +---------------------+

constexpr bool is_dash_dash(std::string_view const whole_arg) noexcept {
  return whole_arg.length() == 2 && whole_arg[0] == '-' && whole_arg[1] == '-';
}

constexpr bool looks_positional(std::string_view const whole_arg) noexcept {
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  return num_of_dashes == 0 || (whole_arg.length() == 1 && num_of_dashes == std::string_view::npos);
}

constexpr std::string_view get_if_short_flags(std::string_view const whole_arg) noexcept {
  auto const names = whole_arg.substr(1);
  auto const all_chars = std::ranges::all_of(names, [](char const c) { return std::isalpha(c); });
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 1 && names.length() >= 1 && all_chars)
    return names;
  return {};
}

constexpr std::string_view get_if_long_flag(std::string_view const whole_arg) noexcept {
  auto const name = whole_arg.substr(2);
  auto const num_of_dashes = whole_arg.find_first_not_of('-');
  if (num_of_dashes == 2 && name.length() >= 2)
    return name;
  return {};
}

} // namespace opz

#endif // OPZIONI_PARSING_H
