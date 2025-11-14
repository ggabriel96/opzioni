#include <print>
#include <variant>

#include "opzioni/all.hpp"

using namespace opz;

int main(int argc, char const *argv[]) {
  constexpr static auto exec_cmd = new_cmd("exec")
                                     .intro("Run a process in a running container")
                                     .pos<"container">({.help = "Name of the target container"})
                                     .pos<"command">({.help = "The command to run in the container"})
                                     .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                                     .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                                     .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"})
                                     .flg<"help", "h">(default_help);

  constexpr static auto pull_cmd = new_cmd("pull")
                                     .intro("Pull an image or a repository from a registry")
                                     .pos<"name">({.help = "The name of the image or repository to pull"})
                                     .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
                                     .flg<"disable-content-trust">({.help = "Skip image verification"})
                                     .opt<"platform", "P">({.help = "Set platform if server is multi-platform capable"})
                                     .flg<"quiet", "q">({.help = "Supress verbose output"})
                                     .flg<"help", "h">(default_help);

  auto docker_cmd =
    new_cmd("docker", "1.0")
      .opt<"config">(
        {.help = "Location of client config files (default: \"{default_value}\")", .default_value = "~/.docker"}
      )
      .flg<"debug", "d", int, act::count>({.help = "Enable debug mode", .implicit_value = 1})
      .flg<"help", "h">(default_help)
      .flg<"version", "v">(default_version)
      .sub(exec_cmd)
      .sub(pull_cmd);

  auto const docker_map = docker_cmd(argc, argv);
  std::print("{} args docker_map (size {}):\n", docker_map.exec_path, docker_map.size());
  std::print("config: {}\n", docker_map.get<"config">());
  std::print("debug: {}\n", docker_map.get<"debug">());

  if (auto const *exec_map = docker_map.get(exec_cmd); exec_map != nullptr) {
    std::print("\n{} args map (size {}):\n", exec_map->exec_path, exec_map->size());
    std::print("container: {}\n", exec_map->get<"container">());
    std::print("command: {}\n", exec_map->get<"command">());
    std::print("detach: {}\n", exec_map->get<"detach">());
    std::print("interactive: {}\n", exec_map->get<"interactive">());
    std::print("tty: {}\n", exec_map->get<"tty">());
    std::print("parsed no further subcommand\n");
  }

  if (auto const *pull_map = docker_map.get(pull_cmd); pull_map != nullptr) {
    std::print("\n{} args map (size {}):\n", pull_map->exec_path, pull_map->size());
    std::print("name: {}\n", pull_map->get<"name">());
    std::print("all-tags: {}\n", pull_map->get<"all-tags">());
    std::print("disable-content-trust: {}\n", pull_map->get<"disable-content-trust">());
    std::print("platform: {}\n", pull_map->get<"platform">());
    std::print("quiet: {}\n", pull_map->get<"quiet">());
    std::print("parsed no further subcommand\n");
  }
}
