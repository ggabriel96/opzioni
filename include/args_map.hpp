#ifndef OPZIONI_ARGS_MAP_H
#define OPZIONI_ARGS_MAP_H

#include <any>
#include <map>
#include <string_view>
#include <variant>

#include "concepts.hpp"
#include "exceptions.hpp"
#include "fixed_string.hpp"
#include "get_type.hpp"
#include "type_list.hpp"

namespace opz {

template <concepts::Cmd>
struct ArgsMap;

template <typename...>
struct ArgsMapOf;
template <concepts::Cmd... Cmds>
struct ArgsMapOf<TypeList<Cmds...>> {
  using type = std::variant<std::monostate, ArgsMap<Cmds const>...>;
};

template <concepts::Cmd Cmd>
struct ArgsMap {
  using cmd_type = Cmd;
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;
  typename ArgsMapOf<typename Cmd::subcmd_types>::type subcmd{};

  template <FixedString Name>
  [[nodiscard]] typename GetType<Name, arg_names, arg_types>::type get() const {
    using T = typename GetType<Name, arg_names, arg_types>::type;
    static_assert(!std::is_same_v<T, void>, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  [[nodiscard]] bool has(std::string_view const name) const noexcept { return args.contains(name); }

  [[nodiscard]] bool has_subcmd() const noexcept { return !std::holds_alternative<std::monostate>(subcmd); }

  [[nodiscard]] auto size() const noexcept { return args.size(); }
};

} // namespace opz

#endif // OPZIONI_ARGS_MAP_H
