#include "converters.hpp"

namespace opzioni {

template <> auto convert<bool>(std::string value) -> bool {
  if (value.empty())
    throw ConversionError("Cannot convert an empty string to bool");
  if (value == "1" || value == "true")
    return true;
  if (value == "0" || value == "false")
    return false;
  throw ConversionError(fmt::format("Cannot convert `{}` to bool", value));
}

template <> auto convert<std::string>(std::string value) -> std::string { return value; }

} // namespace opzioni
