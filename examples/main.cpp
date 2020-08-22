#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using opzioni::actions::append;
  using namespace std::string_literals;

  fmt::print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  opzioni::Program program;
  program.pos("name").help("Your name");
  program.opt("last-name").help("Your last name").otherwise("default value"s);
  program.opt("v").help("Level of verbosity").otherwise(0);
  program.opt("d").help("A double").otherwise(7.11);
  program.flag("flag").help("Long flag");
  program.flag("a").help("Short flag a").otherwise(false);
  program.flag("b").help("Short flag b").otherwise(false);
  program.opt("n").help("A list of numbers").action(append<int>);

  auto subcmd = program.cmd("subcmd").help("Just showing how subcommands work");
  subcmd.pos("subname").help("Your name again, please");
  subcmd.flag("x").help("A nested flag");

  auto const args = program(argc, argv);
  fmt::print("\nCommand name: {}\n", args.cmd_name);
  fmt::print("Number of arguments: {}\n", args.size());
  fmt::print("name: {}\n", args.as<std::string>("name"));
  fmt::print("last name: {}\n", args.as<std::string>("last-name"));
  fmt::print("v: {}\n", args.as<int>("v"));
  fmt::print("d: {}\n", args.as<double>("d"));
  fmt::print("flag: {}\n", args.get_if_else("do something!"s, "flag", "nope"s));
  fmt::print("a: {}\n", args.as<bool>("a"));
  fmt::print("b: {}\n", args.as<bool>("b"));
  fmt::print("n: [{}]\n", fmt::join(args.as<std::vector<int>>("n"), ", "));

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    fmt::print("\nCommand name: {}\n", subargs.cmd_name);
    fmt::print("Number of arguments: {}\n", subargs.size());
    fmt::print("subname: {}\n", subargs["subname"].as<std::string>());
    fmt::print("x: {}\n", subargs.as<bool>("x"));
  }
}
