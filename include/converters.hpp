#ifndef OPZIONI_CONVERTERS_H
#define OPZIONI_CONVERTERS_H

#include <any>
#include <cerrno>
#include <charconv>
#include <concepts>
#include <functional>
#include <optional>
#include <string>

#include <fmt/format.h>

#include "concepts.hpp"
#include "exceptions.hpp"

namespace opz {

using AnyConverter = std::function<std::any(std::optional<std::string>)>;

template <typename TargetType> using TypedConverter = std::function<TargetType(std::optional<std::string>)>;

template <typename TargetType> auto convert(std::optional<std::string>) -> TargetType;

template <Integer Int> auto convert(std::optional<std::string> arg_val) -> Int {
  if (arg_val) {
    Int integer;
    auto const conv_result = std::from_chars(arg_val->data(), arg_val->data() + arg_val->size(), integer);
    if (conv_result.ec == std::errc::invalid_argument)
      throw ConversionError(fmt::format("Cannot convert `{}` to an integer type", *arg_val));
    return integer;
  }
  throw ConversionError("Cannot convert an empty string to int");
}

template <std::floating_point Float> auto convert(std::optional<std::string> arg_val) -> Float {
  if (arg_val) {
    char *end = nullptr;
    Float const floatnum = [&arg_val, &end]() -> Float {
      if constexpr (std::is_same_v<Float, float>) {
        return std::strtof(arg_val->data(), &end);
      } else if constexpr (std::is_same_v<Float, long double>) {
        return std::strtold(arg_val->data(), &end);
      } else {
        // should I explicitly test for double and do something
        // different in the else branch?
        return std::strtod(arg_val->data(), &end);
      }
    }();
    if (errno == ERANGE || end == arg_val->data())
      throw ConversionError(fmt::format("Could not convert `{}` to floating point", *arg_val));
    return floatnum;
  }
  throw ConversionError("Cannot convert an empty string to floating point");
}

} // namespace opz

#endif // OPZIONI_CONVERTERS_H
