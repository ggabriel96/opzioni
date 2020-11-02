#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::Program;
  using opzioni::actions::append;
  using namespace std::string_literals;

  print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  auto program =
      Program("main")
          .help("A short example file to show what can be done with opzioni")
          .details(
              "This is a more detailed description or additional information that goes at the end of the help info");
  program.pos("name").help("Your name");
  program.pos("gather-ints")
      .help("The equivalent of Python's argparse `nargs`, consumes all integers into a vector")
      .gather<int>()
      .otherwise(std::vector{-1});
  program.opt("last-name").help("Your last name");
  program.opt("verbosity", 'v').help("Level of verbosity").set(1).otherwise(0);
  program.opt("double", 'd').help("A double").otherwise(7.11);
  program.flag("flag", 'f')
      .set("do something!"s)
      .otherwise("nope"s)
      .help("The equivalent of Python's argparse `store_const`");
  program.opt("o").help("Testing an option with only a short name").otherwise("oh"s);
  program.flag("t").help("Testing a flag with only a short name").otherwise(false);
  program.flag("append", 'a')
      .set(1)
      .otherwise(std::vector{-1})
      .help("The equivalent of Python's argparse `append_const`")
      .action(append<int>);
  program.opt("num", 'n').help("A list of numbers").action(append<int>);
  program.opt("vec")
      .otherwise(std::vector<int>{})
      .help("Illustrating the difference between appending and assigning to a vector");
  program.opt("str").help("Appends to a list of strings").action(append<std::string>).otherwise(std::vector{""s});

  auto &subcmd = program.cmd("subcmd").help("Just showing how subcommands work");
  subcmd.pos("subname").help("Your name again, please");
  subcmd.flag("ex").help("A nested flag").otherwise(false);

  auto const args = program(argc, argv);
  print("\nCommand name: {}\n", args.cmd_name);
  print("Number of arguments: {}\n", args.size());
  print("name: {}\n", args.as<std::string>("name"));
  print("gather-ints: [{}]\n", fmt::join(args.as<std::vector<int>>("gather-ints"), ", "));
  print("last-name: {}\n", args.has("last-name"s) ? args.as<std::string>("last-name") : "no last name");
  print("verbosity: {}\n", args.as<int>("verbosity"));
  print("t: {}\n", args.as<bool>("t"));
  print("o: {}\n", args.as<std::string>("o"));
  print("double: {}\n", args.as<double>("double"));
  print("flag: {}\n", args.as<std::string>("flag"));
  print("append: {}\n", args.as<std::vector<int>>("append"));
  print("num: [{}]\n", fmt::join(args.as<std::vector<int>>("num"), ", "));
  print("vec: [{}]\n", fmt::join(args.as<std::vector<int>>("vec"), ", "));
  print("str: [{}]\n", fmt::join(args.as<std::vector<std::string>>("str"), ", "));

  if (args.subcmd != nullptr) {
    auto subargs = *args.subcmd;
    print("\nCommand name: {}\n", subargs.cmd_name);
    print("Number of arguments: {}\n", subargs.size());
    print("subname: {}\n", subargs["subname"].as<std::string>());
    print("x: {}\n", subargs.as<bool>("x"));
  }
}
