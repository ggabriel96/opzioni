#ifndef OPZIONI_CONVERTERS_HPP
#define OPZIONI_CONVERTERS_HPP

#include <algorithm>
#include <any>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <ranges>
#include <string_view>

#include <fmt/format.h>

#include "opzioni/concepts.hpp"
#include "opzioni/exceptions.hpp"

namespace opz {

template <typename TargetType>
auto convert(std::string_view) -> TargetType;

template <concepts::Integer Int>
auto convert(std::string_view arg_val) -> Int {
  if (arg_val.empty()) throw ConversionError("empty string", "an integer type");
  Int integer = 0;
  auto const conv_result = std::from_chars(arg_val.data(), arg_val.data() + arg_val.size(), integer);
  if (conv_result.ec == std::errc::invalid_argument) throw ConversionError(arg_val, "an integer type");
  return integer;
}

template <std::floating_point Float>
auto convert(std::string_view arg_val) -> Float {
  if (arg_val.empty()) throw ConversionError("empty string", "a floating point type");
  char *end = nullptr;
  Float const floatnum = [&arg_val, &end]() -> Float {
    if constexpr (std::is_same_v<Float, float>) {
      return std::strtof(arg_val.data(), &end);
    } else if constexpr (std::is_same_v<Float, long double>) {
      return std::strtold(arg_val.data(), &end);
    } else {
      // should I explicitly test for double and do something
      // different in the else branch?
      return std::strtod(arg_val.data(), &end);
    }
  }();
  if (errno == ERANGE || end == arg_val.data()) throw ConversionError(arg_val, "a floating point type");
  return floatnum;
}

template <concepts::Container Container>
auto convert(std::string_view value) -> Container {
  Container container;
  if (!value.empty()) {
    for (auto const val : value | std::views::split(',')) {
      std::string_view const v{val.begin(), val.end()};
      container.emplace_back(convert<typename Container::value_type>(v));
    }
  }
  return container;
}

} // namespace opz

#endif // OPZIONI_CONVERTERS_HPP
