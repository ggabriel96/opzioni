#include <algorithm>
#include <iostream>
#include <sstream>

#include <opzioni.hpp>

namespace opz
{

SplitArg split_arg(std::string const &whole_arg) {
    auto const num_of_dashes = whole_arg.find_first_not_of('-');
    if (num_of_dashes == 0) {
        // no dashes prefix, hence positional argument
        return SplitArg{
            .num_of_dashes = 0,
            .name = "",
            .value = whole_arg
        };
    }
    if (num_of_dashes == std::string::npos) {
        // all dashes, hence disables future parsing if length is 2
        return SplitArg{
            .num_of_dashes = whole_arg.length(),
            .name = "",
            .value = std::nullopt
        };
    }
    auto const eq_idx = whole_arg.find('=', num_of_dashes);
    if (eq_idx == std::string::npos) {
        // has dashes prefix, but no equals,
        // hence either long flag or long option with next CLI argument as value
        auto const name = whole_arg.substr(num_of_dashes);
        return SplitArg{
            .num_of_dashes = num_of_dashes,
            .name = name,
            .value = std::nullopt
        };
    }
    // has dashes prefix and equals, hence long option
    auto const name = whole_arg.substr(num_of_dashes, eq_idx - num_of_dashes);
    auto const value = whole_arg.substr(eq_idx + 1);
    return SplitArg{
        .num_of_dashes = num_of_dashes,
        .name = name,
        .value = value
    };
}

auto ArgParser::get_remaining_args(int start_idx, int argc, char const *argv[]) const {
    auto remaining_args = std::make_unique<std::vector<std::string>>();
    int const remaining_args_count = argc - start_idx;
    remaining_args->reserve(remaining_args_count);
    std::copy_n(argv + start_idx, remaining_args_count, std::back_inserter(*remaining_args));
    return remaining_args;
}

ArgMap ArgParser::parse_args(int argc, char const *argv[]) const
{
    ArgMap arg_map;
    int positional_idx = 0;
    std::cout << "> parse_args\n";
    for (int i = 1; i < argc; ++i)
    {
        auto const whole_arg = std::string(argv[i]);
        std::cout << "\n>> whole_arg: " << whole_arg << '\n';

        auto const split = split_arg(whole_arg);
        std::cout << ">> split_arg: " << split.num_of_dashes << ' ' << split.name << ' ' << (split.value ? *split.value : "nullopt") << '\n';
        if (split.num_of_dashes == 2 && split.name.empty()) {
            arg_map.remaining_args = get_remaining_args(i + 1, argc, argv);
            break;
        } else if (split.num_of_dashes == 1 && split.name.length() > 1 && !split.value) {
            // support for arguments like -abc where a, b and c are flags
            // TODO: extract into separate function!?
            try {
                std::for_each(split.name.begin(), split.name.end(), [this, &arg_map](char const &c) {
                    auto const flag_arg = this->options.at(std::string(1, c));
                    arg_map.args[flag_arg.name] = ParsedArg{flag_arg.flag_value};
                });
                continue;
            } catch (std::out_of_range const &) {
                // support for arguments like -v2, where v is name and 2 is value
            }
        } else if (split.num_of_dashes == 0) {
            auto const positional_arg = this->positional_args[positional_idx];
            auto const parsed_value = positional_arg.converter(split.value);
            arg_map.args[positional_arg.name] = ParsedArg{parsed_value};
            ++positional_idx;
            continue;
        }
        auto const arg = this->options.at(split.name);
        std::cout << ">> arg: " << arg.help << '\n';
        if (arg.flag_value.has_value()) {
            std::cout << ">> flag_value\n";
            if (split.value.has_value())
                throw std::invalid_argument("Flag argument must not have a value");
            arg_map.args[split.name] = ParsedArg{arg.flag_value};
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
