#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::Opt, opzioni::Pos;

  auto program =
      opzioni::Program("gather").intro("A short example file to illustrate the gather feature").auto_help() +
      Pos("gather-all")
          .help(
              "This is the equivalent of Python's argparse `nargs` with value `+`: it requires at least one value and "
              "consumes all of them into a vector. Note that this exact type of argument is rather limiting because, "
              "as it consumes every argument when it is its turn in the parsing process, it will not allow us to parse "
              "any subsequent positional or command argument (options and flags may appear before it, though)")
          .gather<int>() +
      Opt("gather-2")
          .help("This is similar to the previous gather, but it limits the amount of arguments to only 2,"
                " so it is not so problematic")
          .gather<int>(2);

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("gather-all: {}\n", args.has("gather-all") ? args.as<std::vector<int>>("gather-all") : std::vector<int>{});
  print("gather-2: {}\n", args.has("gather-2") ? args.as<std::vector<int>>("gather-2") : std::vector<int>{});
}
