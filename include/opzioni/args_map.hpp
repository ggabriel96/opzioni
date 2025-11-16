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
#include "opzioni/string_list.hpp"
#include "opzioni/type_list.hpp"
#include "opzioni/variant.hpp"

namespace opz {

struct StringsMap;

struct StringsMap {
  // vector because each option may have N values
  using ValueType = std::vector<std::string_view>;

  std::string_view exec_path{};
  ValueType positionals;
  std::map<std::string_view, ValueType> options;
  std::map<std::string_view, std::size_t> flags;
};

template <concepts::Cmd> struct ArgsMap;

template <typename...> struct ArgsMapOf;
template <concepts::Cmd... Cmds> struct ArgsMapOf<TypeList<Cmds...>> {
  using type = std::variant<empty, ArgsMap<Cmds const>...>;
  // `const` above so that consumers may use decltype(cmd) without `std::remove_const_t` and friends
};

template <typename...> struct TupleOf;
template <typename... Ts>
struct TupleOf<TypeList<Ts...>> {
  using type = std::tuple<std::optional<Ts>...>;
};

template <concepts::Cmd Cmd>
struct ArgsMap {
  using cmd_type = Cmd;

  std::string_view exec_path{};
  std::map<std::string_view, std::any> args;
  typename TupleOf<typename Cmd::arg_types>::type t_args;
  typename ArgsMapOf<typename Cmd::subcmd_types>::type submap{};

  template <FixedString Name>
  [[nodiscard]] typename GetType<Name, typename cmd_type::arg_names, typename cmd_type::arg_types>::type get() const {
    using T = typename GetType<Name, typename cmd_type::arg_names, typename cmd_type::arg_types>::type;
    static_assert(!std::is_same_v<T, void>, "unknown parameter name");
    auto const val = args.find(Name);
    if (val == args.end()) throw ArgumentNotFound(Name.data);
    return std::any_cast<T>(val->second);
  }

  template <concepts::Cmd SubCmd>
  [[nodiscard]] ArgsMap<SubCmd const> const *get(SubCmd const &) const noexcept {
    return std::get_if<ArgsMap<SubCmd const>>(&submap);
  }

  template <FixedString Name>
  [[nodiscard]] constexpr int idx_of() const noexcept {
    return IndexOfStr<0, Name, typename Cmd::arg_names>::value;
  }

  template <int Idx>
  [[nodiscard]] bool has_value() const noexcept {
    return std::get<Idx>(this->t_args).has_value();
  }

  template <FixedString Name>
  [[nodiscard]] bool has_value() const noexcept {
    constexpr auto idx = this->idx_of<Name>();
    return this->has_value<idx>();
  }

  [[nodiscard]] bool has_submap() const noexcept { return !std::holds_alternative<empty>(submap); }

  // TODO: size() doens't make sense anymore since t_args is always "full-size"
  // can we have a set member function to set values in t_args and in that increment a counter?
  [[nodiscard]] auto size() const noexcept { return args.size(); }
};

} // namespace opz

#endif // OPZIONI_ARGS_MAP_HPP
