#include <print>
#include <variant>

#include "all.hpp"

using namespace opz;

constexpr static auto hello_cmd = new_cmd("hello", "2.0").pos<"name">({});
void handle_pull_subcmd(ArgsMap<decltype(hello_cmd)> const &map) {
  std::print("\nh\nargs map (size {}):\n", map.size());
  std::print("name: {}\n", map.get<"name">());
}

constexpr static auto exec_cmd = new_cmd("exec", "1.0")
                                   .intro("Run a process in a running container")
                                   .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                                   .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                                   .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"});
void handle_pull_subcmd(ArgsMap<decltype(exec_cmd)> const &map) {
  std::print("\ne\nargs map (size {}):\n", map.size());
  std::print("detach: {}\n", map.get<"detach">());
  std::print("interactive: {}\n", map.get<"interactive">());
  std::print("tty: {}\n", map.get<"tty">());
}

constexpr static auto pull_cmd = new_cmd("pull", "1.0")
                                   .intro("Pull an image or a repository from a registry")
                                   .pos<"name">({.help = "The name of the image or repository to pull"})
                                   .opt<"platform", "P">({.help = "Set platform if server is multi-platform capable"})
                                   .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
                                   .flg<"disable-content-trust">({.help = "Skip image verification"})
                                   .flg<"quiet", "q">({.help = "Supress verbose output"})
                                   .with(exec_cmd)
                                   .with(hello_cmd);
void handle_docker_subcmd(ArgsMap<decltype(pull_cmd)> const &map) {
  std::print("\np\nargs map (size {}):\n", map.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("quiet: {}\n", map.get<"quiet">());

  std::visit(
    overloaded{
      [](std::monostate) { std::print("monostate\n"); },
      [](auto const &sub_map) { handle_pull_subcmd(sub_map); },
    },
    map.subcmd);
}

int main(int argc, char const *argv[]) {
  auto docker = new_cmd("docker", "1.0").with(pull_cmd);
  auto const map = parse(docker, argc, argv);
  std::print("\nargs map (size {}):\n", map.size());

  std::visit(
    overloaded{
      [](std::monostate) { std::print("monostate\n"); },
      [](auto const &sub_map) { handle_docker_subcmd(sub_map); },
    },
    map.subcmd);
}
