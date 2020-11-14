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

  //   print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  auto program = Program("opzioni's main example")
                     .intro("A short example file illustrating opzioni's simpler features")
                     .details("This example only covers simple positionals, options, and flags. For examples of more "
                              "complicated parse actions or subcommands, please take a look at the other examples.");

  program.pos("name").help("Your first name");

  program.opt("double", 'd').help("A double").otherwise(7.11);
  program.opt("last-name").help("Your last name");
  program.opt("o").help("We also support options with only short names").otherwise("oh"s);
  program.opt("num", 'n').help("Creates a list of numbers with each appearence of this argument").action(append<int>);
  program.opt("str").help("Appends to a list of strings").action(append<std::string>).otherwise(std::vector{""s});
  program.opt("vec")
      .otherwise(std::vector<int>{})
      .help("This option uses the default action, so it'll not append to a list from each appearence in the CLI. It "
            "will instead parse a comma-separated list once");
  program.opt("verbose", 'v').help("Level of verbosity").set(1).otherwise(0);

  program.flag("append", 'a')
      .set(1)
      .otherwise(std::vector{-1})
      .help("The equivalent of Python's argparse `append_const`: will append the defined value every time it appears "
            "in the CLI")
      .action(append<int>);
  program.flag("flag", 'f')
      .set("do something!"s)
      .otherwise("nope"s)
      .help("The equivalent of Python's argparse `store_const`: will store the defined value if it appears in the CLI");
  program.flag("t").help("We also support flags with only short names").otherwise(false);

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("name: {}\n", args.as<std::string>("name"));

  print("double: {}\n", args.as<double>("double"));
  print("last-name: {}\n", args.has("last-name"s) ? args.as<std::string>("last-name") : "no last name");
  print("o: {}\n", args.as<std::string>("o"));
  print("num: [{}]\n", fmt::join(args.as<std::vector<int>>("num"), ", "));
  print("str: [{}]\n", fmt::join(args.as<std::vector<std::string>>("str"), ", "));
  print("vec: [{}]\n", fmt::join(args.as<std::vector<int>>("vec"), ", "));
  print("verbose: {}\n", args.as<int>("verbose"));

  print("append: {}\n", args.as<std::vector<int>>("append"));
  print("flag: {}\n", args.as<std::string>("flag"));
  print("t: {}\n", args.as<bool>("t"));
}
