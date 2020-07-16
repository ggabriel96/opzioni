#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using opzioni::Arg;

  fmt::print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  opzioni::Program program;
  program.pos({.name = "name", .help = "Your name"});
  program.opt({.name = "last-name", .help = "Your last name"});
  program.opt({.name = "v", .help = "Level of verbosity"});
  program.opt({.name = "d", .help = "A double"});
  program.flag({.name = "flag", .terse = "f", .help = "Long flag"});
  program.flag({.name = "a", .help = "Short flag a"});
  program.flag({.name = "b", .help = "Short flag b"});
  program.opt({.name = "numbers", .help = "A list of numbers"});

  auto subcmd = program.cmd({.name = "subcmd"});
  subcmd->pos({.name = "subname", .help = "Your name again, please"});
  subcmd->flag({.name = "x", .help = "A nested flag"});

  auto const args = program.parse(argc, argv);
  fmt::print("\nCommand name: {}\n", args.cmd_name);
  fmt::print("Number of arguments: {}\n", args.size());
  fmt::print("name: {}\n", args["name"].as<std::string>());
  fmt::print("last name: {}\n", args["last-name"].as<std::string>());
  fmt::print("v: {}\n", args["v"].as<std::string>());
  fmt::print("d: {}\n", args["d"].as<std::string>());
  fmt::print("flag: {}\n", args["flag"].as<std::string>());
  fmt::print("a: {}\n", args["a"].as<std::string>());
  fmt::print("b: {}\n", args["b"].as<std::string>());
  fmt::print("numbers: [{}]\n", fmt::join(args["numbers"].as<std::vector<int>>(), ", "));

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    fmt::print("\nCommand name: {}\n", subargs.cmd_name);
    fmt::print("Number of arguments: {}\n", subargs.size());

    // alternative way to extract argument values using `operator T`
    std::string subname = subargs["subname"];
    bool x = subargs["x"];
    fmt::print("subname: {}\n", subname);
    fmt::print("x: {}\n", x);
  }
}
