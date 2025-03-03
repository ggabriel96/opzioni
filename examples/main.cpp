#include "all.hpp"

int main(int argc, char const *argv[]) {
  using namespace opz;

  auto p = new_cmd("pull", "1.0")
             .intro("Pull an image or a repository from a registry")
             .pos<"name">({.help = "The name of the image or repository to pull"})
             .opt<"platform">({.help = "Set platform if server is multi-platform capable"})
             .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
             .flg<"disable-content-trust">({.help = "Skip image verification"})
             .flg<"quiet", "q">({.help = "Supress verbose output"});

  auto const map = parse(p, argc, argv);
  std::print("\nargs map (size {}):\n", map.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("quiet: {}\n", map.get<"quiet">());
}
