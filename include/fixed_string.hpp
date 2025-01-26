#ifndef FIXED_STRING_H
#define FIXED_STRING_H

#include <cstddef>

struct construct_from_pointer_t {};
constexpr auto construct_from_pointer = construct_from_pointer_t{};

template <size_t N>
struct fixed_string {
  char data[N];

  constexpr fixed_string(char const input[N], construct_from_pointer_t) {
    for (size_t i{0}; i < N; ++i) {
      data[i] = input[i];
    }
  }

  constexpr fixed_string(char const (&input)[N + 1]) noexcept : fixed_string{input, construct_from_pointer} {}
};

template <size_t N>
fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

#endif