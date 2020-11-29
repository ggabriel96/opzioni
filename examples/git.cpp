#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using namespace std::string_literals;

  auto git = opzioni::program("git CLI simulation, v0.1")
                 .intro("An example to illustrate subcommands via simulation of the git interface")
                 .details("See 'git <command> --help' to read about a specific subcommand.");

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
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());
  print("Sub-command name, if present: {}\n", args.cmd_name);

  if (auto const clone = args.cmd("clone")) {
    print("\nSub-command path: {}\n", clone->exec_path);
    print("Number of arguments: {}\n", clone->size());
    print("Sub-command name, if present: {}\n", clone->cmd_name);
    print("repository: {}\n", clone->as<std::string>("repository"));
    print("directory: {}\n", clone->as<std::string>("directory"));
  }
}
