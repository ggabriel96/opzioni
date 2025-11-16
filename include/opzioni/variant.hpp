#ifndef OPZIONI_VARIANT_HPP
#define OPZIONI_VARIANT_HPP

#include "opzioni/type_list.hpp"

#include <variant>

namespace opz {

struct empty {};

template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename...> struct VariantOf;
template <typename... Ts>
struct VariantOf<TypeList<Ts...>> {
  using type = std::variant<Ts...>;
};

} // namespace opz

#endif // OPZIONI_VARIANT_HPP
