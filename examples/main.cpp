#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "experimental/all.hpp"

int main(int argc, char const *argv[]) {
    auto p = Program()
        .Pos<"name", std::string>({.help = "The name of the delivery recipient"})
        .Pos<"address", std::string>({.help = "The shipping address for your purchase"})
    ;

    // p.SetValue<"age">(28);
    // auto age = p.GetValue<"name">();
    // fmt::print("name: [{}]\n", age.value_or(std::string()));
}
