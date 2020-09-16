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
  program.opt("last-name").help("Your last name");
  program.opt("v").help("Level of verbosity").set(1).otherwise(0);
  program.opt("d").help("A double").otherwise(7.11);
  program.flag("flag")
      .set("do something!"s)
      .otherwise("nope"s)
      .help("The equivalent of Python's argparse `store_const`");
  program.flag("f")
      .set(1)
      .otherwise(std::vector{-1})
      .help("The equivalent of Python's argparse `append_const`")
      .action(append<int>);
  program.flag("a").help("Short flag a").otherwise(false);
  program.flag("b").help("Short flag b").otherwise(false);
  program.opt("n").help("A list of numbers").action(append<int>);
  program.opt("vec")
      .otherwise(std::vector<int>{})
      .help("Illustrating the difference between appending and assigning to a vector");
  program.pos("test").help("The equivalent of Python's argparse `nargs`").gather<int>().otherwise(std::vector{-1});
  program.opt("str").action(append<std::string>).otherwise(std::vector{""s});

  auto &subcmd = program.cmd("subcmd").help("Just showing how subcommands work");
  subcmd.pos("subname").help("Your name again, please");
  subcmd.flag("x").help("A nested flag").otherwise(false);

  auto const args = program(argc, argv);
  fmt::print("\nCommand name: {}\n", args.cmd_name);
  fmt::print("Number of arguments: {}\n", args.size());
  fmt::print("name: {}\n", args.as<std::string>("name"));
  fmt::print("last name: {}\n", args.has("last-name"s) ? args.as<std::string>("last-name") : "no last name");
  fmt::print("v: {}\n", args.as<int>("v"));
  fmt::print("d: {}\n", args.as<double>("d"));
  fmt::print("flag: {}\n", args.as<std::string>("flag"));
  fmt::print("f: {}\n", args.as<std::vector<int>>("f"));
  fmt::print("a: {}\n", args.as<bool>("a"));
  fmt::print("b: {}\n", args.as<bool>("b"));
  fmt::print("n: [{}]\n", fmt::join(args.as<std::vector<int>>("n"), ", "));
  fmt::print("vec: [{}]\n", fmt::join(args.as<std::vector<int>>("vec"), ", "));
  fmt::print("test: [{}]\n", fmt::join(args.as<std::vector<int>>("test"), ", "));
  fmt::print("str: [{}]\n", fmt::join(args.as<std::vector<std::string>>("str"), ", "));

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    fmt::print("\nCommand name: {}\n", subargs.cmd_name);
    fmt::print("Number of arguments: {}\n", subargs.size());
    fmt::print("subname: {}\n", subargs["subname"].as<std::string>());
    fmt::print("x: {}\n", subargs.as<bool>("x"));
  }
}
