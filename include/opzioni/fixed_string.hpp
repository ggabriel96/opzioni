#ifndef OPZIONI_FIXED_STRING_HPP
#define OPZIONI_FIXED_STRING_HPP

#include <cstddef>
#include <string_view>

namespace opz {

template <std::size_t N>
struct FixedString {
  char data[N + 1]; // hold space for '\0'
  std::size_t size = N;

  constexpr FixedString(char const *input) noexcept {
    for (std::size_t i{0}; i < N; ++i) {
      data[i] = input[i];
    }
    data[N] = '\0';
  }

  constexpr char operator[](std::size_t idx) const noexcept {
    return this->data[idx];
  }

  // string_view does not generally store the trailing '\0'
  constexpr operator std::string_view() const noexcept { return {data, N}; }
};

template <std::size_t N>
FixedString(char const (&)[N]) -> FixedString<N - 1>;

} // namespace opz

#endif // OPZIONI_FIXED_STRING_HPP
