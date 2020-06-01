#ifndef OPZIONI_CONCEPTS_H
#define OPZIONI_CONCEPTS_H

#include <concepts>

namespace opz {

template <typename T>
concept Integer = std::integral<T> && !std::same_as<T, bool>;

} // namespace opz

#endif // OPZIONI_CONCEPTS_H
