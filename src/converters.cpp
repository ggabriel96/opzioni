#include "converters.hpp"

namespace opz {

template <>
auto convert<bool>(std::string_view value) -> bool {
  if (value.empty())
    throw ConversionError("empty string", "bool");
  if (value == "1" || value == "true")
    return true;
  if (value == "0" || value == "false")
    return false;
  throw ConversionError(value, "bool");
}

template <>
auto convert<std::string_view>(std::string_view value) -> std::string_view {
  return std::string_view(value);
}

template <>
auto convert<std::string>(std::string_view value) -> std::string {
  // TODO (?): remove this and force usage of string_view?
  return std::string(value);
}

} // namespace opz
