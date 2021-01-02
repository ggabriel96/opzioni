#ifndef OPZIONI_CONCEPTS_H
#define OPZIONI_CONCEPTS_H

#include <concepts>
#include <ranges>
#include <type_traits>
#include <variant>
#include <vector>
#include <string_view>

namespace opzioni {

// +----------------------------------------+
// | related to type list and builtin types |
// +----------------------------------------+

template <typename...>
struct TypeList;

template <typename...>
struct InList : std::false_type {};

template <typename T, typename... Ts>
struct InList<T, TypeList<T, Ts...>> : std::true_type {};

template <typename T, typename U, typename... Ts>
struct InList<T, TypeList<U, Ts...>> : InList<T, TypeList<Ts...>> {};

template <typename...>
struct Concat;

template <typename... Lhs, typename... Rhs>
struct Concat<TypeList<Lhs...>, TypeList<Rhs...>> {
  using type = TypeList<Lhs..., Rhs...>;
};

template <typename...>
struct VariantOf;

template <typename... Ts>
struct VariantOf<TypeList<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename...>
struct VectorOf;

template <typename... Ts>
struct VectorOf<TypeList<Ts...>> {
  using type = TypeList<std::vector<Ts>...>;
};

// types that may be used as default_value and set_value
using BuiltinTypes = TypeList<std::monostate, bool, int, double, std::string_view>;
using BuiltinVariant = VariantOf<BuiltinTypes>::type;

// types that may be the result of parsing the CLI
using ExternalTypes = Concat<BuiltinTypes, VectorOf<BuiltinTypes>::type>::type;
using ExternalVariant = VariantOf<ExternalTypes>::type;

// +----------+
// | concepts |
// +----------+

namespace concepts {

template <typename T>
concept BuiltinType = InList<T, BuiltinTypes>::value;

template <typename T>
concept ExternalType = InList<T, ExternalTypes>::value;

template <typename T>
concept Integer = std::integral<T> && !std::same_as<T, bool>;

template <typename T>
concept Container = std::ranges::range<T> &&std::is_default_constructible_v<T> &&requires(T t) {
  typename T::value_type;
  { t.emplace_back(std::declval<typename T::value_type>()) }
  ->std::same_as<typename T::value_type &>;
};

} // namespace concepts
} // namespace opzioni

#endif // OPZIONI_CONCEPTS_H
