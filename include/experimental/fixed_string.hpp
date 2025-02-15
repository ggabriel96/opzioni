#ifndef OPZIONI_FIXED_STRING_H
#define OPZIONI_FIXED_STRING_H

#include <cstddef>
#include <string_view>

template <size_t N>
struct fixed_string {
  char data[N];

  constexpr fixed_string(char const (&input)[N]) {
    for (size_t i{0}; i < N; ++i) {
      data[i] = input[i];
    }
  }

  constexpr operator std::string_view() const noexcept { return data; }
};

#endif