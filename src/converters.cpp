#include "converters.hpp"

namespace opzioni {

template <>
auto convert<bool>(std::string_view value) -> bool {
  if (value.empty())
    throw ConversionError("Cannot convert an empty string to bool");
  if (value == "1" || value == "true")
    return true;
  if (value == "0" || value == "false")
    return false;
  throw ConversionError(fmt::format("Cannot convert `{}` to bool", value));
}

template <>
auto convert<std::string>(std::string_view value) -> std::string {
  return std::string(value);
}

} // namespace opzioni
