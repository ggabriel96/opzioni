#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "experimental/all.hpp"

int main(int argc, char const *argv[]) {
    auto p = DefaultProgram("pull")
        .Pos<"name", std::string>({.help = "The name of the image or repository to pull"})
        .Opt<"platform", std::string>({.help = "Set platform if server is multi-platform capable"})
        .Flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
        .Flg<"disable-content-trust">({.help = "Skip image verification"})
        .Flg<"quiet", "q">({.help = "Supress verbose output"})
    ;

    // p.SetValue<"age">(28);
    // auto age = p.GetValue<"name">();
    // fmt::print("name: [{}]\n", age.value_or(std::string()));
}
