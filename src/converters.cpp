#include <sstream>

#include <opzioni.hpp>

namespace opz
{

    template <>
    auto convert<bool>(std::optional<std::string> value) -> bool
    {
        if (value)
        {
            if (*value == "1" || *value == "true")
                return true;
            if (*value == "0" || *value == "false")
                return false;
            throw std::invalid_argument("Invalud boolean value");
        }
        throw std::invalid_argument("Cannot convert nonexistant boolean");
    }

    template <>
    auto convert<int>(std::optional<std::string> arg_val) -> int
    {
        if (arg_val)
            return std::stoi(*arg_val);
        throw std::invalid_argument("Cannot convert nonexistant number");
    }

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
        if (!value)
            return v;
        std::string token;
        std::istringstream token_stream(*value);
        while (std::getline(token_stream, token, ','))
        {
            v.emplace_back(convert<int>(token));
        }
        return v;
    }

} // namespace opz
