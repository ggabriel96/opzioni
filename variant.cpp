#include <charconv>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>

using namespace std;
using namespace std::string_literals;

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template <typename T> struct Arg {
  string name{};
  optional<T> otherwise = nullopt;

  string parsed_value{};
  T converted_value{};
};

using BuiltInTypes = variant<Arg<string>, Arg<int>, Arg<bool>>;

template <typename T> void convert(Arg<T> &);

template <> void convert<bool>(Arg<bool> &ba) {
  if (ba.parsed_value.empty()) {
    if (ba.otherwise)
      ba.converted_value = *ba.otherwise;
    else
      throw std::invalid_argument("Cannot convert an empty string to bool");
  } else {
    if (ba.parsed_value == "1" || ba.parsed_value == "true")
      ba.converted_value = true;
    else if (ba.parsed_value == "0" || ba.parsed_value == "false")
      ba.converted_value = false;
    else
      throw std::invalid_argument(fmt::format("Cannot convert `{}` to bool", ba.parsed_value));
  }
}

template <> void convert<std::string>(Arg<string> &sa) { sa.converted_value = sa.parsed_value; }

template <> void convert<int>(Arg<int> &ia) {
  if (ia.parsed_value.empty()) {
    if (ia.otherwise)
      ia.converted_value = *ia.otherwise;
    else
      throw std::invalid_argument("Cannot convert an empty string to an integer type");
  } else {
    int integer;
    auto const conv_result = std::from_chars(ia.parsed_value.data(), ia.parsed_value.data() + ia.parsed_value.size(), integer);
    if (conv_result.ec == std::errc::invalid_argument)
      throw std::invalid_argument(fmt::format("Could not convert `{}` to an integer type", ia.parsed_value));
    ia.converted_value = integer;
  }
}

struct Program {
  map<string, BuiltInTypes> args;

  template <typename T> void pos(Arg<T> &&arg) { args[arg.name] = arg; }
};

int main() {
  Program p;
  p.pos<int>({.name = "arg"s, .otherwise = 11, .parsed_value = ""});

  cout << "converted before: ";
  std::visit([](auto &&arg) { cout << arg.converted_value << '\n'; }, p.args["arg"]);

  std::visit([](auto &&arg) { convert(arg); }, p.args["arg"]);
  
  cout << "converted after: ";
  std::visit([](auto &&arg) { cout << arg.converted_value << '\n'; }, p.args["arg"]);
}
