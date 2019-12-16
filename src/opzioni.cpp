#include <algorithm>
#include <iostream>
#include <sstream>

#include <opzioni.hpp>

namespace opz
{

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
        std::cout << ">> split_arg: " << split.num_of_dashes << ' ' << split.name << ' ' << (split.value ? *split.value : "nullopt") << '\n';
        if (split.num_of_dashes == 2 && split.name.empty()) {
            if (positional_after_dashes) {
                throw std::invalid_argument("Only positional arguments are allowed after \"--\"");
            }
            positional_after_dashes = true;
            continue;
        } else if (split.num_of_dashes == 1 && split.name.length() > 1 && !split.value) {
            // support for arguments like -abc where a, b and c are flags
            // TODO: extract into separate function!?
            std::for_each(split.name.begin(), split.name.end(), [this, &arg_map](char const &c) {
                auto const flag_arg = this->get_arg(std::string(1, c));
                arg_map.args[flag_arg.name] = flag_arg.flag_value;
            });
            continue;
        }
        auto const arg = this->get_arg(split.name);
        std::cout << ">> get_arg: " << arg.help << '\n';
        if (arg.flag_value.has_value()) {
            std::cout << ">> flag_value\n";
            if (split.value.has_value())
                throw std::invalid_argument("Flag argument must not have a value");
            arg_map.args[split.name] = arg.flag_value;
        } else {
            std::cout << ">> not flag_value\n";
            auto const arg_value = [&]() {
                if (split.value) return *split.value;
                if (i + 1 < argc) {
                    ++i;
                    return std::string(argv[i]);
                } else {
                    throw std::invalid_argument("Missing required argument");
                }
            }();
            std::cout << ">> arg_value: " << arg_value << '\n';
            
            auto const parsed_value = arg.converter(arg_value);
            arg_map.args[split.name] = ParsedArg{parsed_value};
        }
    }
    return arg_map;
}

} // namespace opz
