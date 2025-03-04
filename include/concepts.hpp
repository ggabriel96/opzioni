#ifndef OPZIONI_CONCEPTS_H
#define OPZIONI_CONCEPTS_H

#include <ranges>

namespace opz::concepts {

template <typename T>
concept Integer = std::integral<T> && !std::same_as<T, bool>;

template <typename T>
concept Container = std::ranges::range<T> && std::is_default_constructible_v<T> && requires(T t) {
  typename T::value_type;
  { t.emplace_back(std::declval<typename T::value_type>()) } -> std::same_as<typename T::value_type &>;
};

template <typename T>
concept Command = requires(T t) {
  typename T::arg_names;
  typename T::arg_types;
};

} // namespace opz::concepts

#endif // OPZIONI_CONCEPTS_H
