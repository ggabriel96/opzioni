#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::Program, opzioni::Pos, opzioni::Opt;

  auto program =
      Program("gather").auto_version("1.0").intro("A short example file to illustrate the gather feature").auto_help() +
      Pos("gather-all")
          .help("This is the equivalent of Python's argparse `nargs` with `+`: it requires at least one value and "
                "consumes all of them into a vector. Note that precisely this type of argument is somewhat limiting "
                "because, since it consumes every argument, it will not allow us to parse anything that comes after it")
          .gather<int>() +
      Opt("gather-2")
          .help("This is similar to the previous gather, but it limits the amount of consumed arguments to only 2,"
                " hence it is not so problematic")
          .gather<int>(2);

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("gather-all: {}\n", args.as<std::vector<int>>("gather-all"));
  print("gather-2: {}\n", args.as<std::vector<int>>("gather-2"));
}
