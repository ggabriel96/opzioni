#ifndef OPZIONI_ARGS_MAP_HPP
#define OPZIONI_ARGS_MAP_HPP

#include <any>
#include <map>
#include <string_view>
#include <type_traits>
#include <variant>

#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/fixed_string.hpp"
#include "opzioni/get_type.hpp"
#include "opzioni/type_list.hpp"
#include "opzioni/variant.hpp"

namespace opz {

template <concepts::Cmd> struct ArgsMap;

template <typename...> struct ArgsMapOf;
template <concepts::Cmd... Cmds> struct ArgsMapOf<TypeList<Cmds...>> {
  using type = std::variant<empty, ArgsMap<Cmds const>...>;
  // `const` above so that consumers may use decltype(cmd) without `std::remove_const_t` and friends
};

template <typename...> struct TupleOf;
template <typename ...Ts>
struct TupleOf<TypeList<Ts...>> {
  using type = std::tuple<std::optional<Ts>...>;
};

template <concepts::Cmd Cmd>
struct ArgsMap {
  using cmd_type = Cmd;
  using arg_names = typename Cmd::arg_names;
  using arg_types = typename Cmd::arg_types;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;
  // typename TupleOf<arg_types>::type argsx;
  typename ArgsMapOf<typename Cmd::subcmd_types>::type submap{};

  template <FixedString Name>
  [[nodiscard]] typename GetType<Name, arg_names, arg_types>::type get() const {
    using T = typename GetType<Name, arg_names, arg_types>::type;
    static_assert(!std::is_same_v<T, void>, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  template <concepts::Cmd SubCmd>
  [[nodiscard]] ArgsMap<SubCmd const> const *get(SubCmd const &) const noexcept {
    return std::get_if<ArgsMap<SubCmd const>>(&submap);
  }

  [[nodiscard]] bool has(std::string_view const name) const noexcept { return args.contains(name); }

  [[nodiscard]] bool has_submap() const noexcept { return !std::holds_alternative<empty>(submap); }

  [[nodiscard]] auto size() const noexcept { return args.size(); }
};

} // namespace opz

#endif // OPZIONI_ARGS_MAP_HPP
