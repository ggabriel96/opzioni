#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include <concepts>

namespace opz {

template <typename T>
concept Integer = std::integral<T> && !std::same_as<T, bool>;

} // namespace opz

#endif // OPZIONI_TYPES_H
