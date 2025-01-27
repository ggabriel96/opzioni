#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "experimental/exp.hpp"

int main(int argc, char const *argv[]) {
    auto p = Program()
        .Add<"age", int>()
        .Add<"name", std::string>();

    p.SetValue<"age">(28);
    auto age = p.GetValue<"age">();
    fmt::print("age: [{}]\n", age.value_or(-1));
}
