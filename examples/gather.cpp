#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using namespace opzioni;

  constexpr auto program =
      Program("gather", "A short example file to illustrate the gather feature")
          .version("1.0")
          .add(Help())
          .add(Version())
          .add(Pos("all")
                   .help("This is the equivalent of Python's argparse `nargs` with `+`: it requires at least one value "
                         "and consumes all of them into a vector. Note that precisely this type of argument is "
                         "somewhat limiting because, since it consumes every argument, it will not allow us to parse "
                         "anything that comes after it")
                   .action(Append<int>().gather()))
          .add(Opt("two")
                   .help("This is similar to the previous gather, but it limits the amount of consumed arguments to "
                         "only {gather_amount}, hence it is not so problematic. Default: {default_value}")
                   .action(Append<int>().gather(2).otherwise(+[](ArgValue &arg) {
                     arg.value = std::vector{0, 0};
                   })));

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("all: {}\n", args.as<std::vector<std::string_view>>("all"));
  print("two: {}\n", args.as<std::vector<int>>("two"));
}
