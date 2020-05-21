#ifndef OPZIONI_H
#define OPZIONI_H

#include <any>
#include <charconv>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <utility>

namespace opz
{

    class ArgMap;
    class ArgValue;

    using AnyConverter = std::function<std::any(std::optional<std::string>)>;

    template <typename TargetType>
    using TypedConverter = std::function<TargetType(std::optional<std::string>)>;

    template <typename TargetType>
    auto convert(std::optional<std::string>) -> TargetType;

    template <typename T>
    struct Arg
    {
        std::string name;
        std::string help_text;
        bool is_required = false;
        std::set<T> choices;
        std::optional<T> default_value = std::nullopt;
        std::optional<T> flag_value = std::nullopt;
        AnyConverter converter = opz::convert<T>;

        Arg(std::string name) noexcept : name(name) {}

        Arg<T> &help(std::string help_text) noexcept
        {
            this->help_text = help_text;
            return *this;
        }

        Arg<T> &required() noexcept
        {
            this->is_required = true;
            return *this;
        }

        Arg<T> &not_required() noexcept
        {
            this->is_required = false;
            return *this;
        }

        Arg<T> &among(std::set<T> choices) noexcept
        {
            this->choices = choices;
            return *this;
        }

        Arg<T> &with_default(T default_value) noexcept
        {
            this->default_value = default_value;
            return *this;
        }

        Arg<bool> &as_flag() noexcept
        {
            this->flag_value = true;
            return with_default(false);
        }

        Arg<T> &as_flag(T flag_value) noexcept
        {
            this->flag_value = flag_value;
            return *this;
        }

        Arg<T> &converted_by(AnyConverter converter) noexcept
        {
            this->converter = converter;
            return *this;
        }
    };

    struct ArgInfo
    {
        std::string name;
        std::string help;
        bool required = false;
        std::any default_value;
        std::any flag_value;
        AnyConverter converter;

        bool is_flag() const
        {
            return this->flag_value.has_value();
        }
    };

    struct SplitArg
    {
        size_t num_of_dashes;
        std::string name;
        std::optional<std::string> value;
    };

    template <typename TargetType>
    TargetType apply_conversion(AnyConverter const &convert_fn, std::optional<std::string> const &str_value)
    {
        return std::any_cast<TargetType>(convert_fn(str_value));
    }

    template <typename TargetType>
    TargetType apply_conversion(TypedConverter<TargetType> const &convert_fn, std::optional<std::string> const &str_value)
    {
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
        void add(Arg<T> spec)
        {
            if (!spec.choices.empty())
                add_choice_checking_to_conversion(spec);
            auto const num_of_dashes = spec.name.find_first_not_of('-');
            // static_assert(
            //     num_of_dashes == 0 || spec.default_value.has_value() || spec.is_required,
            //     "An argument must either be positional, have a default value, or be required"
            // );
            if (num_of_dashes != std::string::npos && num_of_dashes > 0)
                spec.name = spec.name.substr(num_of_dashes);
            auto const arg_info = ArgInfo{
                .name = spec.name,
                .help = spec.help_text,
                .required = spec.is_required,
                .default_value = spec.default_value ? *spec.default_value : std::any{},
                .flag_value = spec.flag_value ? *spec.flag_value : std::any{},
                .converter = spec.converter};
            if (num_of_dashes == 0)
                this->positional_args.push_back(arg_info);
            else
                this->options[spec.name] = arg_info;
        }

        ArgMap parse_args(int, char const *[]) const;

    private:
        std::vector<ArgInfo> positional_args;
        std::unordered_map<std::string, ArgInfo> options;

        bool is_multiple_short_flags(std::string const &) const;

        template <typename T>
        void add_choice_checking_to_conversion(Arg<T> &spec) const noexcept
        {
            spec.converter = [choices = spec.choices,
                              converter = spec.converter](std::optional<std::string> value) -> std::any {
                auto const result = apply_conversion<T>(converter, value);
                if (!choices.contains(result))
                    throw std::invalid_argument("Provided value is not in the set of possible values");
                return result;
            };
        }
    };

    struct ArgValue
    {
        std::any value;

        template <typename TargetType>
        TargetType as() const
        {
            return std::any_cast<TargetType>(this->value);
        }
    };

    class ArgMap
    {
    public:
        ArgValue const &operator[](std::string arg_name) const { return args.at(arg_name); }

        auto size() const noexcept
        {
            return this->args.size();
        }

        std::unique_ptr<std::vector<std::string>> remaining_args;

    private:
        std::unordered_map<std::string, ArgValue> args;

        void set_defaults_for_missing_options(std::unordered_map<std::string, ArgInfo> const &);

        friend ArgMap ArgParser::parse_args(int, char const *[]) const;
    };

} // namespace opz

#endif // OPZIONI_H
