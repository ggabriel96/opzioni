#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using fmt::print;
  using namespace opzioni;

  constexpr auto program =
      Program("main")
          .version("1.0")
          .intro("A short example illustrating opzioni's simpler features")
          .details("This example only covers simple positionals, options, and flags. For examples of more"
                   " complicated parse actions or subcommands, please take a look at the other examples.")
          .add_args(
              Help() * Version() * Pos("name").help("Your first name") *
              Opt("double", "d").help("A double. Default: {default_value}")[act::Assign<double>().otherwise(7.11)] *
              Opt("last-name").help("Your last name") *
              Opt("o").help(
                  "We also support having short names only. Default: {default_value}")[act::Assign().otherwise("oh")] *
              Opt("num", "n")
                  .help("Creates a vector of numbers with each appearence of this argument. Default: {default_value}")
                      [act::Append<int>()] *
              Opt("csv").help("In contrast to `Append`, this will create a vector of numbers from a single "
                              "comma-separated list of values. Default: {default_value}")[act::List<int>()] *
              Opt("verbose", "v")
                  .help("Level of verbosity. "
                        "Sets to {implicit_value} if given without a value (e.g. -{abbrev}). Default: {default_value}")
                      [act::Assign<int>().implicitly(1).otherwise(0)] *
              Flg("append", "a")
                  .help(
                      "The equivalent of Python's argparse `append_const`: will append {implicit_value} every time it "
                      "appears in the CLI. Default: {default_value}")[act::Append<int>().implicitly(1)] *
              Flg("flag", "f")
                  .help("The equivalent of Python's argparse `store_const`: will store \"{implicit_value}\" if it "
                        "appears in "
                        "the CLI. Default: {default_value}")
                      [act::Assign().implicitly("do something!").otherwise("nope")] *
              Counter("t").help("We also support flags with only short names. This argument counts how many times it "
                                "appears in the CLI. Default: {default_value}") *
              Opt("woo", "w").help("Woo")[act::Append<int>().gather(3)]);

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
  print("woo: {}\n", args.as<std::vector<int>>("woo"));
  print("verbose: {}\n", args.as<int>("verbose"));

  print("append: {}\n", args.has("append") ? args.as<std::vector<int>>("append") : std::vector<int>{});
  print("flag: {}\n", args.as<std::string_view>("flag"));
  print("t: {}\n", args.as<std::size_t>("t"));
}
