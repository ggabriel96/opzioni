#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::Program;

  auto program = Program("opzioni's gather example").intro("A short example file to illustrate the gather feature");

  program.pos("gather-all")
      .help("This is the equivalent of Python's argparse `nargs` with value `+`: it requires at least one value and "
            "consumes all of them into a vector. Note that this exact type of argument is rather limiting because, as "
            "it consumes every argument when it is its turn in the parsing process, it will not allow us to parse any "
            "subsequent positional or command argument (options and flags may appear before it, though)")
      .gather<int>()
      .otherwise(std::vector<int>{});

  program.opt("gather-2")
      .help("This is similar to the previous gather, but it limits the amount of arguments to only 2, so it is not so "
            "problematic")
      .gather<int>(2)
      .otherwise(std::vector<int>{});

  auto const args = program(argc, argv);
  print("\nCommand name: {}\n", args.cmd_name);
  print("Command path: {}\n", args.cmd_path);
  print("Number of arguments: {}\n", args.size());

  print("gather-all: {}\n", args.as<std::vector<int>>("gather-all"));
  print("gather-2: {}\n", args.as<std::vector<int>>("gather-2"));
}
