#ifndef OPZIONI_ARGS_MAP_HPP
#define OPZIONI_ARGS_MAP_HPP

#include <string_view>
#include <variant>

#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/fixed_string.hpp"
#include "opzioni/get_type.hpp"
#include "opzioni/string_list.hpp"
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
template <typename... Ts>
struct TupleOf<TypeList<Ts...>> {
  using type = std::tuple<std::optional<Ts>...>;
};

template <concepts::Cmd Cmd>
struct ArgsMap {
  using cmd_type = Cmd;

  std::string_view exec_path{};
  TupleOf<typename Cmd::arg_types>::type args;
  ArgsMapOf<typename Cmd::subcmd_types>::type submap{};

  template <FixedString Name>
  [[nodiscard]] GetType<Name, typename cmd_type::arg_names, typename cmd_type::arg_types>::type get() const {
    constexpr auto idx = this->idx_of<Name>();
    auto const arg = std::get<idx>(args);
    if (!arg) throw ArgumentNotFound(Name.data);
    return *arg;
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
    return std::get<Idx>(this->args).has_value();
  }

  template <FixedString Name>
  [[nodiscard]] bool has_value() const noexcept {
    constexpr auto idx = this->idx_of<Name>();
    return this->has_value<idx>();
  }

  [[nodiscard]] bool has_submap() const noexcept { return !std::holds_alternative<empty>(submap); }
};

} // namespace opz

#endif // OPZIONI_ARGS_MAP_HPP
