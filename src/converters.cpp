#include <sstream>

#include <opzioni.hpp>

namespace opz
{

template <>
auto convert<std::string>(std::optional<std::string> value) -> std::string
{
    return value.value_or("");
}

// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
template <>
auto convert<std::vector<int>>(std::optional<std::string> value) -> std::vector<int>
{
    std::vector<int> v;
    if (!value) return v;
    std::string token;
    std::istringstream token_stream(value.value());
    while (std::getline(token_stream, token, ',')) {
        v.emplace_back(convert<int>(token));
    }
    return v;
}

} // namespace opz
