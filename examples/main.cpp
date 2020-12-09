#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using opzioni::actions::append;
  using namespace std::string_view_literals;
  using opzioni::flg, opzioni::opt, opzioni::pos;

  auto program =
      opzioni::Program("main")
          .intro("A short example file illustrating opzioni's simpler features")
          .details("This example only covers simple positionals, options, and flags. For examples of more"
                   " complicated parse actions or subcommands, please take a look at the other examples.")
          .auto_help() +
      pos("name").help("Your first name") + opt("double", "d").help("A double").otherwise(7.11) +
      opt("last-name").help("Your last name") +
      opt("o").help("We also support options with only short names").otherwise("oh"sv) +
      opt("num", "n").help("Creates a list of numbers with each appearence of this argument").action(append<int>) +
      opt("str").help("Appends to a list of strings").action(append<std::string_view>).otherwise(std::vector{""sv}) +
      opt("vec")
          .otherwise(std::vector<int>{})
          .help("This option uses the default action, so it'll not append to a list from each appearence in the CLI."
                " It will instead parse a comma-separated list once") +
      opt("verbose", "v").help("Level of verbosity").set(1).otherwise(0) +
      flg("append", "a")
          .set(1)
          .otherwise(std::vector{-1})
          .help("The equivalent of Python's argparse `append_const`:"
                " will append the defined value every time it appears in the CLI")
          .action(append<int>) +
      flg("flag", "f")
          .set("do something!"sv)
          .otherwise("nope"sv)
          .help("The equivalent of Python's argparse `store_const`:"
                " will store the defined value if it appears in the CLI") +
      flg("t").help("We also support flags with only short names").otherwise(false);

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("name: {}\n", args.as<std::string_view>("name"));

  print("double: {}\n", args.as<double>("double"));
  print("last-name: {}\n", args.has("last-name"sv) ? args.as<std::string_view>("last-name") : "no last name");
  print("o: {}\n", args.as<std::string_view>("o"));
  print("num: [{}]\n", fmt::join(args.as<std::vector<int>>("num"), ", "));
  print("str: [{}]\n", fmt::join(args.as<std::vector<std::string_view>>("str"), ", "));
  print("vec: [{}]\n", fmt::join(args.as<std::vector<int>>("vec"), ", "));
  print("verbose: {}\n", args.as<int>("verbose"));

  print("append: {}\n", args.as<std::vector<int>>("append"));
  print("flag: {}\n", args.as<std::string_view>("flag"));
  print("t: {}\n", args.as<bool>("t"));
}
