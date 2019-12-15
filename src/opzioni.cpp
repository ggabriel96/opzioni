#include <algorithm>
#include <iostream>
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

SplitArg split_arg(std::string const &whole_arg) {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    if (eq_idx == std::string::npos) {
        auto const name = whole_arg.substr(num_of_dashes);
        return SplitArg{
            .num_of_dashes = num_of_dashes,
            .name = name,
            .value = std::nullopt
        };
    }
    auto const name = whole_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
    auto const value = whole_arg.substr(eq_idx + 1);
    return SplitArg{
        .num_of_dashes = num_of_dashes,
        .name = name,
        .value = value
    };
}

ArgInfo ArgParser::get_arg(std::string const &name) const {
    auto const arg_idx = this->index.at(name);
    return this->arguments[arg_idx];
}

ArgMap ArgParser::parse_args(int argc, char const *argv[]) const
{
    ArgMap arg_map;
    bool positional_after_dashes = false;
    std::cout << "> parse_args\n";
    for (int i = 1; i < argc; ++i)
    {
        auto const whole_arg = std::string(argv[i]);
        std::cout << "\n>> whole_arg: " << whole_arg << '\n';

        auto const split = split_arg(whole_arg);
        std::cout << ">> split_arg: " << split.num_of_dashes << ' ' << split.name << ' ' << (split.value ? split.value.value() : "nullopt") << '\n';
        if (split.num_of_dashes == 2 && split.name.empty()) {
            if (positional_after_dashes) {
                throw std::invalid_argument("Only positional arguments are allowed after \"--\"");
            }
            positional_after_dashes = true;
            continue;
        }
        auto const arg = this->get_arg(split.name);
        std::cout << ">> get_arg: " << arg.help << '\n';
        if (arg.required && !split.value) {
            throw std::invalid_argument("Argument \"" + arg.name + "\" is required");
        }
        auto const parsed_value = arg.converter(split.value);
        arg_map.args[split.name] = ParsedArg{parsed_value};
    }
    return arg_map;
}

} // namespace opz
