#ifndef OPZIONI_CONVERTERS_H
#define OPZIONI_CONVERTERS_H

#include "concepts.hpp"
#include "exceptions.hpp"

#include <any>
#include <cerrno>
#include <charconv>
#include <concepts>
#include <optional>
#include <sstream>
#include <string>

#include <fmt/format.h>

namespace opzioni {

template <typename TargetType> auto convert(std::string) -> TargetType;

template <concepts::Integer Int> auto convert(std::string arg_val) -> Int {
  if (arg_val.empty())
    throw ConversionError("Cannot convert an empty string to an integer type");
  Int integer;
  auto const conv_result = std::from_chars(arg_val.data(), arg_val.data() + arg_val.size(), integer);
  if (conv_result.ec == std::errc::invalid_argument)
    throw ConversionError(fmt::format("Could not convert `{}` to an integer type", arg_val));
  return integer;
}

template <std::floating_point Float> auto convert(std::string arg_val) -> Float {
  if (arg_val.empty())
    throw ConversionError("Cannot convert an empty string to a floating point type");
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
  if (errno == ERANGE || end == arg_val.data())
    throw ConversionError(fmt::format("Could not convert `{}` to a floating point type", arg_val));
  return floatnum;
}

// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
template <typename Container>
auto convert(std::string value) -> Container
    requires(std::is_same_v<Container, std::vector<typename Container::value_type>>) {
  Container container;
  if (value.empty())
    return container;
  std::string token;
  std::istringstream token_stream(value);
  while (std::getline(token_stream, token, ',')) {
    container.emplace_back(convert<typename Container::value_type>(token));
  }
  return container;
}

} // namespace opzioni

#endif // OPZIONI_CONVERTERS_H
