#include <print>
#include <variant>

#include "opzioni/all.hpp"

using namespace opz;

constexpr static auto exec_cmd = new_cmd("exec")
                                   .intro("Run a process in a running container")
                                   .pos<"container">({.help = "Name of the target container"})
                                   .pos<"command">({.help = "The command to run in the container"})
                                   .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                                   .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                                   .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"})
                                   .flg<"help", "h">(default_help);

void handle_docker_subcmd(ArgsMap<decltype(exec_cmd)> const &map) {
  std::print("\n{} args map (size {}):\n", map.exec_path, map.size());
  std::print("container: {}\n", map.get<"container">());
  std::print("command: {}\n", map.get<"command">());
  std::print("detach: {}\n", map.get<"detach">());
  std::print("interactive: {}\n", map.get<"interactive">());
  std::print("tty: {}\n", map.get<"tty">());
  std::visit(
    overloaded{
      [](opz::empty) { std::print("parsed no further subcommand\n"); },
    },
    map.subcmd
  );
}

constexpr static auto pull_cmd = new_cmd("pull")
                                   .intro("Pull an image or a repository from a registry")
                                   .pos<"name">({.help = "The name of the image or repository to pull"})
                                   .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
                                   .flg<"disable-content-trust">({.help = "Skip image verification"})
                                   .opt<"platform", "P">({.help = "Set platform if server is multi-platform capable"})
                                   .flg<"quiet", "q">({.help = "Supress verbose output"})
                                   .flg<"help", "h">(default_help);

void handle_docker_subcmd(ArgsMap<decltype(pull_cmd)> const &map) {
  std::print("\n{} args map (size {}):\n", map.exec_path, map.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("quiet: {}\n", map.get<"quiet">());
  std::visit(
    overloaded{
      [](opz::empty) { std::print("parsed no further subcommand\n"); },
    },
    map.subcmd
  );
}

int main(int argc, char const *argv[]) {
  auto docker_cmd =
    new_cmd("docker", "1.0")
      .opt<"config">(
        {.help = "Location of client config files (default {default_value})", .default_value = "~/.docker"}
      )
      .flg<"debug", "D", int>({.help = "Enable debug mode", .implicit_value = 1, .action = act::count})
      .flg<"help", "h">(default_help)
      .flg<"version", "v">(default_version)
      .sub(exec_cmd)
      .sub(pull_cmd);

  auto const map = docker_cmd(argc, argv);
  std::print("{} args map (size {}):\n", map.exec_path, map.size());
  std::print("config: {}\n", map.get<"config">());
  std::print("debug: {}\n", map.get<"debug">());
  std::visit(
    overloaded{
      [](opz::empty) { std::print("parsed no further subcommand\n"); },
      [](auto const &sub_map) { handle_docker_subcmd(sub_map); },
    },
    map.subcmd
  );
}
