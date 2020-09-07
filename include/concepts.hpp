#ifndef OPZIONI_CONCEPTS_H
#define OPZIONI_CONCEPTS_H

#include <concepts>

namespace opzioni {

namespace concepts {

template <typename T> concept Integer = std::integral<T> && !std::same_as<T, bool>;

} // namespace concepts
} // namespace opzioni

#endif // OPZIONI_CONCEPTS_H
