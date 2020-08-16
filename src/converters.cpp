#include "converters.hpp"

#include <sstream>

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

// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
template <> auto convert<std::vector<int>>(std::string value) -> std::vector<int> {
  std::vector<int> v;
  if (value.empty())
    return v;
  std::string token;
  std::istringstream token_stream(value);
  while (std::getline(token_stream, token, ',')) {
    v.emplace_back(convert<int>(token));
  }
  return v;
}

} // namespace opzioni
