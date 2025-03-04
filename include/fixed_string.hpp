#ifndef OPZIONI_FIXED_STRING_H
#define OPZIONI_FIXED_STRING_H

#include <cstddef>
#include <string_view>

namespace opz {

template <size_t N>
struct FixedString {
  char data[N];
  std::size_t size = N;

  constexpr FixedString(char const *input) noexcept {
    for (size_t i{0}; i < N; ++i) {
      data[i] = input[i];
    }
  }

  constexpr operator std::string_view() const noexcept { return {data, N}; }
};

template <size_t N>
FixedString(char const (&)[N]) -> FixedString<N - 1>;

} // namespace opz

#endif // OPZIONI_FIXED_STRING_H
