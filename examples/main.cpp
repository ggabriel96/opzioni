#include <print>
#include <variant>

#include "all.hpp"

using namespace opz;

constexpr static auto exec_cmd = new_cmd("exec", "1.0")
                                   .intro("Run a process in a running container")
                                   .pos<"container">({.help = "Name of the target container"})
                                   .pos<"command">({.help = "The command to run in the container"}) // TODO: use append+gather
                                   .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                                   .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                                   .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"});

void handle_docker_subcmd(ArgsMap<decltype(exec_cmd)> const &map) {
  std::print("\n{} args map (size {}):\n", map.exec_path, map.size());
  std::print("container: {}\n", map.get<"container">());
  std::print("command: {}\n", map.get<"command">());
  std::print("detach: {}\n", map.get<"detach">());
  std::print("interactive: {}\n", map.get<"interactive">());
  std::print("tty: {}\n", map.get<"tty">());
  std::visit(
    overloaded{
      [](std::monostate) { std::print("parsed no further subcommand\n"); },
    },
    map.subcmd);
}

constexpr static auto pull_cmd = new_cmd("pull", "1.0")
                                   .intro("Pull an image or a repository from a registry")
                                   .pos<"name">({.help = "The name of the image or repository to pull"})
                                   .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
                                   .flg<"disable-content-trust">({.help = "Skip image verification"})
                                   .opt<"platform", "P">({.help = "Set platform if server is multi-platform capable"})
                                   .flg<"quiet", "q">({.help = "Supress verbose output"});

void handle_docker_subcmd(ArgsMap<decltype(pull_cmd)> const &map) {
  std::print("\n{} args map (size {}):\n", map.exec_path, map.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("quiet: {}\n", map.get<"quiet">());
  std::visit(
    overloaded{
      [](std::monostate) { std::print("parsed no further subcommand\n"); },
    },
    map.subcmd);
}

int main(int argc, char const *argv[]) {
  auto docker_cmd = new_cmd("docker", "1.0")
    .opt<"config">({.help = "Location of client config files (default {default_value})", .default_value = "~/.docker"})
    .flg<"debug", "D">({.help = "Enable debug mode"})
    .with(exec_cmd)
    .with(pull_cmd);

  auto const map = parse(docker_cmd, argc, argv);
  std::print("\n{} args map (size {}):\n", map.exec_path, map.size());
  std::print("config: {}\n", map.get<"config">());
  std::print("debug: {}\n", map.get<"debug">());
  std::visit(
    overloaded{
      [](std::monostate) { std::print("parsed no further subcommand\n"); },
      [](auto const &sub_map) { handle_docker_subcmd(sub_map); },
    },
    map.subcmd);
}
