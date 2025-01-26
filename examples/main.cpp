#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "exp.hpp"

int main(int argc, char const *argv[]) {
    // constexpr auto p = Program<StringList<"age", "name">, TypeList<int, std::string>>();
    constexpr auto p = Program()
        .Add<"age", int>()
        .Add<"name", std::string>();

    auto name = p.GetValue<"name">();
    fmt::print("name: [{}]\n", name);
}
