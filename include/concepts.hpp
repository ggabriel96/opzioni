#ifndef OPZIONI_CONCEPTS_H
#define OPZIONI_CONCEPTS_H

#include <concepts>

namespace opzioni {

template <typename T> concept Integer = std::integral<T> && !std::same_as<T, bool>;

} // namespace opzioni

#endif // OPZIONI_CONCEPTS_H
