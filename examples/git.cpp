#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::Program;
  using namespace std::string_literals;

  auto git =
      Program("opzioni's simulation of the git CLI")
          .intro("An example to illustrate subcommands via simulation of the git interface")
          .details(
              "This is a more detailed description or additional information that goes at the end of the help info");

  auto &clone = git.cmd("clone").intro("Clone a repository into a new directory");
  clone.pos("repository")
      .help("The (possibly remote) repository to clone from. See the GIT URLS section below for more information on "
            "specifying repositories.");
  clone.pos("directory")
      .help("The name of a new directory to clone into. The \"humanish\" part of the source repository is used if no "
            "directory is explicitly given (repo for /path/to/repo.git and foo for host.xz:foo/.git). Cloning into an "
            "existing directory is only allowed if the directory is empty.")
      .otherwise(""s);

  auto const args = git(argc, argv);
  print("\nCommand name: {}\n", args.cmd_name);
  print("Command path: {}\n", args.cmd_path);
  print("Number of arguments: {}\n", args.size());

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    print("\nSub-command name: {}\n", subargs.cmd_name);
    print("Sub-command path: {}\n", subargs.cmd_path);
    print("Number of arguments: {}\n", subargs.size());
    if (subargs.cmd_name == "clone"s) {
      print("repository: {}\n", subargs["repository"].as<std::string>());
      print("directory: {}\n", subargs.as<std::string>("directory"));
    }
  }
}
