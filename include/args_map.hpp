#ifndef OPZIONI_ARGS_MAP_H
#define OPZIONI_ARGS_MAP_H

#include <any>
#include <map>
#include <string_view>
#include <type_traits>
#include <variant>

#include "concepts.hpp"
#include "exceptions.hpp"
#include "fixed_string.hpp"
#include "get_type.hpp"
#include "type_list.hpp"

namespace opz {

template <concepts::Command>
struct ArgsMap;

template <typename...>
struct ArgsMapOf;
template <concepts::Command... Cmds>
struct ArgsMapOf<TypeList<Cmds...>> {
  using type = std::variant<std::monostate, ArgsMap<Cmds const>...>;
};

template <concepts::Command Cmd>
struct ArgsMap {
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;
  typename ArgsMapOf<typename Cmd::sub_cmd_types>::type sub_cmd{};

  template <FixedString Name>
  typename GetType<Name, arg_names, arg_types>::type get() const {
    using T = typename GetType<Name, arg_names, arg_types>::type;
    static_assert(!std::is_same_v<T, void>, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  // TODO: get_or(<default value>)

  bool has(std::string_view name) const noexcept { return args.contains(name); }

  auto size() const noexcept { return args.size(); }
};

} // namespace opz

#endif // OPZIONI_ARGS_MAP_H
