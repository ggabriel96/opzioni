#ifndef OPZIONI_H
#define OPZIONI_H

#include <any>
#include <charconv>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <utility>

namespace opz
{

class ArgMap;
class ParsedArg;

using AnyConverter = std::function<std::any(std::optional<std::string>)>;

template <typename TargetType>
using TypedConverter = std::function<TargetType(std::optional<std::string>)>;

template <typename TargetType>
auto convert(std::optional<std::string>) -> TargetType;

template <typename T>
struct ArgSpec
{
    std::string name;
    std::string help;
    std::set<T> choices;
    bool required = false;
    AnyConverter converter = opz::convert<T>;
    std::optional<T> default_value = std::nullopt;
    std::optional<T> implicit_value = std::nullopt;
};

template<typename T>
ArgSpec(std::string, std::string, std::set<T>) -> ArgSpec<T>;

template<typename T>
ArgSpec(std::string, std::string, std::set<T>, T) -> ArgSpec<T>;

template<typename T>
ArgSpec(std::string, std::string, std::set<T>, std::optional<T>) -> ArgSpec<T>;

template<typename T>
ArgSpec(std::string, std::string, std::set<T>, bool, AnyConverter, T) -> ArgSpec<T>;

template<typename T>
ArgSpec(std::string, std::string, std::set<T>, bool, AnyConverter, std::optional<T>) -> ArgSpec<T>;

struct SplitArg
{
    size_t num_of_dashes;
    std::string name;
    std::optional<std::string> value;
};

struct ArgInfo
{
    std::string name;
    std::string help;
    bool required = false;
    std::any default_value;
    AnyConverter converter;
};


template <typename TargetType>
TargetType apply_conversion(AnyConverter const &convert_fn, std::optional<std::string> const &str_value) {
    return std::any_cast<TargetType>(convert_fn(str_value));
}

template <typename TargetType>
TargetType apply_conversion(TypedConverter<TargetType> const &convert_fn, std::optional<std::string> const &str_value) {
    return convert_fn(str_value);
}

/**
 * exec name
 * description
 * epilog
 * 
 * allow unknown arguments
 */
class ArgParser
{
public:
    template <typename T>
    void add_arg(ArgSpec<T> spec)
    {
        if (!spec.choices.empty() || spec.implicit_value) {
            spec.converter = [
                choices = spec.choices,
                converter = spec.converter,
                implicit_value = spec.implicit_value
            ](std::optional<std::string> value) -> std::any {
                if (!value && implicit_value) return implicit_value.value();
                auto const result = apply_conversion<T>(converter, value);
                if (!choices.empty() && !choices.contains(result)) {
                    throw std::invalid_argument("Provided value is not in the set of possible values");
                }
                return result;
            };
        }
        spec.name = [&name = spec.name]() {
            auto const num_of_dashes = name.find_first_not_of('-');
            return name.substr(num_of_dashes);
        }();
        // TODO: deduce action from T, e.g. `flag` if T = bool
        this->index[spec.name] = this->arguments.size();
        this->arguments.emplace_back(ArgInfo{
            .name = spec.name,
            .help = spec.help,
            .required = spec.required,
            .default_value = spec.default_value,
            .converter = spec.converter
        });
    }
    

    ArgMap parse_args(int, char const *[]) const;
    ArgInfo get_arg(std::string const &) const;

private:
    std::vector<ArgInfo> arguments;
    std::unordered_map<std::string, int> index;
};

struct ParsedArg
{
public:
    ParsedArg() = default;
    ParsedArg(std::any value) : value(value) {}

    template <typename TargetType>
    TargetType as() const
    {
        return std::any_cast<TargetType>(this->value);
    }

private:
    std::any value;
};

class ArgMap
{
public:
    ParsedArg const &operator[](std::string arg_name) const { return args.at(arg_name); }

    template <typename TargetType>
    TargetType operator()(std::string arg_name) const
    {
        return args.at(arg_name).as<TargetType>();
    }

    auto size() const noexcept {
        return this->args.size();
    }

private:
    std::unordered_map<std::string, ParsedArg> args;

    friend ArgMap ArgParser::parse_args(int, char const *[]) const;
};

} // namespace opz

#endif // OPZIONI_H
