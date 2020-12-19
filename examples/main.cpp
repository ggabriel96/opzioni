#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using namespace std::string_view_literals;
  using opzioni::Program, opzioni::Flg, opzioni::Opt, opzioni::Pos;

  auto program =
      Program("main")
          .intro("A short example illustrating opzioni's simpler features")
          .details("This example only covers simple positionals, options, and flags. For examples of more"
                   " complicated parse actions or subcommands, please take a look at the other examples.")
          .auto_help() +
      Pos("name").help("Your first name") + Opt("double", "d").help("A double").otherwise(7.11) +
      Opt("last-name").help("Your last name") +
      Opt("o").help("We also support options with only short names").otherwise("oh"sv) +
      Opt("num", "n").append<int>().help("Creates a vector of numbers with each appearence of this argument") +
      Opt("csv").csv_of<int>().help("In contrast to `append`, this will create a vector of numbers from a single "
                                    "comma-separated list of values") +
      Opt("verbose", "v").help("Level of verbosity").set(1).otherwise(0) +
      Flg("append", "a")
          .set(1)
          .append<int>()
          .help("The equivalent of Python's argparse `append_const`:"
                " will append the defined value every time it appears in the CLI") +
      Flg("flag", "f")
          .set("do something!"sv)
          .otherwise("nope"sv)
          .help("The equivalent of Python's argparse `store_const`:"
                " will store the defined value if it appears in the CLI") +
      Flg("t").help("We also support flags with only short names").otherwise(false);

  auto const args = program(argc, argv);
  print("\nCommand path: {}\n", args.exec_path);
  print("Number of arguments: {}\n", args.size());

  print("name: {}\n", args.as<std::string_view>("name"));

  print("double: {}\n", args.as<double>("double"));
  // illustrating how one could handle arguments without default values
  print("last-name: {}\n", args.has("last-name") ? args.as<std::string_view>("last-name") : "no last name");
  print("o: {}\n", args.as<std::string_view>("o"));
  print("num: {}\n", args.as<std::vector<int>>("num"));
  print("csv: {}\n", args.as<std::vector<int>>("csv"));
  print("verbose: {}\n", args.as<int>("verbose"));

  print("{}\n", args.has("append"));
  print("append: {}\n", args.has("append") ? args.as<std::vector<int>>("append") : std::vector<int>{});
  print("flag: {}\n", args.as<std::string_view>("flag"));
  print("t: {}\n", args.as<bool>("t"));
}
