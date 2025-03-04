#include <type_traits>
#include <variant>

#include "all.hpp"

void exec_cmd(auto const &parent_map, std::span<char const *> const rest_args) {
  using namespace opz;
  auto exec = new_cmd("exec", "1.0")
                .intro("Run a process in a running container")
                .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"});
  // auto const map = parse(exec, rest_args);
  // std::print("\nargs map (size {}):\n", map.size());
  // std::print("detach: {}\n", map.get<"detach">());
  // std::print("interactive: {}\n", map.get<"interactive">());
  // std::print("tty: {}\n", map.get<"tty">());
}

int main(int argc, char const *argv[]) {
  using namespace opz;

  constexpr static auto exec = new_cmd("exec", "1.0")
                                 .intro("Run a process in a running container")
                                 .flg<"detach", "d">({.help = "Detached mode: run command in the background"})
                                 .flg<"interactive", "i">({.help = "Keep STDIN open even if not attached"})
                                 .flg<"tty", "t">({.help = "Allocate a pseudo-TTY"});

  auto pull = new_cmd("pull", "1.0")
                .intro("Pull an image or a repository from a registry")
                .pos<"name">({.help = "The name of the image or repository to pull"})
                .opt<"platform">({.help = "Set platform if server is multi-platform capable"})
                .flg<"all-tags", "a">({.help = "Download all tagged images in the repository"})
                .flg<"disable-content-trust">({.help = "Skip image verification"})
                .flg<"quiet", "q">({.help = "Supress verbose output"})
                .with(exec);

  // std::variant<std::monostate, CommandParser<std::tuple_element_t<0, decltype(pull.sub_cmds)>>> v;
  // auto parser = CommandParser(pull);
  // v = parser;
  // std::print(">>> {}\n", std::get<CommandParser<decltype(pull)>>(v).cmd_ref.get().name);

  auto const map = parse(pull, argc, argv);
  std::print("\nargs map (size {}):\n", map.size());
  std::print("name: {}\n", map.get<"name">());
  std::print("platform: {}\n", map.get<"platform">());
  std::print("all-tags: {}\n", map.get<"all-tags">());
  std::print("disable-content-trust: {}\n", map.get<"disable-content-trust">());
  std::print("quiet: {}\n", map.get<"quiet">());
}
