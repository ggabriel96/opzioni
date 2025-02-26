#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "experimental/all.hpp"

int main(int argc, char const *argv[]) {
  auto p = DefaultProgram("pull", "1.0")
               .Intro("Pull an image or a repository from a registry")
               .Pos<"name", std::string>({.help = "The name of the image or repository to pull"})
               .Opt<"platform", std::string>({.help = "Set platform if server is multi-platform capable"})
               .Flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
               .Flg<"disable-content-trust">({.help = "Skip image verification"})
               .Flg<"quiet", "q">({.help = "Supress verbose output"});

  // auto const r = FindArg(p.args, [](auto const view) { return view.name == "platform"; });
  // std::print("{}\n", r->name);

  auto const map = parse(p, argc, argv);
  std::print("\nargs map (size {}):\n", map.args.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("quiet: {}\n", map.get<"quiet">());

  // p.SetValue<"age">(28);
  // auto age = p.GetValue<"name">();
  // fmt::print("name: [{}]\n", age.value_or(std::string()));
}
