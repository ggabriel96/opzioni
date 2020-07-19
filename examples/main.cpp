#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using namespace std::string_literals;

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
  fmt::print("last name: {}\n", args.value_or("last-name", ""s));
  fmt::print("v: {}\n", args.value_or("v", 0));
  fmt::print("d: {}\n", args.value_or("d", 0.0));
  fmt::print("flag: {}\n", args.get_if_else("do something!"s, "flag", "nope"s));
  fmt::print("a: {}\n", args.value_or("a", false));
  fmt::print("b: {}\n", args.value_or("b", false));
  fmt::print("numbers: [{}]\n", fmt::join(args.value_or("numbers", std::vector<int>{}), ", "));

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    fmt::print("\nCommand name: {}\n", subargs.cmd_name);
    fmt::print("Number of arguments: {}\n", subargs.size());
    fmt::print("subname: {}\n", subargs["subname"].as<std::string>());
    fmt::print("x: {}\n", subargs.value_or("x", false));
  }
}
