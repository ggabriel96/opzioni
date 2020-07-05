#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using opzioni::Arg;

  fmt::print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  opzioni::Program program;
  program.add(Arg<std::string>("name").help("Your name"));
  program.add(Arg<std::string>("--last-name").help("Your last name"));
  program.add(Arg<int>("-v").help("Level of verbosity").among({0, 1, 2, 3, 4}).with_default(0));
  program.add(Arg<double>("-d").help("A double"));
  program.add(Arg<int>("--flag").help("Long flag").with_default(7).as_flag(11));
  program.add(Arg<bool>("-a").help("Short flag a").with_default(false).as_flag(true));
  program.add(Arg<bool>("-b").help("Short flag b").as_flag());
  program.add(Arg<std::vector<int>>("--numbers").help("A list of numbers"));

  auto subcmd = program.sub({.name = "subcmd"});
  subcmd->add(Arg<std::string>("subname").help("Your name again, please"));
  subcmd->add(Arg<bool>("-x").help("A nested flag").as_flag());

  auto const args = program.parse(argc, argv);
  fmt::print("\nNumber of arguments: {}\n", args.size());
  fmt::print("name: {}\n", args["name"].as<std::string>());
  fmt::print("last name: {}\n", args["last-name"].as<std::string>());
  fmt::print("v: {}\n", args["v"].as<int>());
  fmt::print("d: {}\n", args["d"].as<double>());
  fmt::print("flag: {}\n", args["flag"].as<int>());
  fmt::print("a: {}\n", args["a"].as<bool>());
  fmt::print("b: {}\n", args["b"].as<bool>());
  fmt::print("numbers: [{}]\n", fmt::join(args["numbers"].as<std::vector<int>>(), ", "));

  if (args.sub != nullptr) {
    auto sub = args.sub.get();
    fmt::print("\nSub-argument parsed: {}\n", sub->sub_name);
    fmt::print("Number of sub-arguments: {}\n", sub->size());
    fmt::print("subname: {}\n", (*sub)["subname"].as<std::string>());
    fmt::print("x: {}\n", sub->args["x"].as<bool>());
  }
}
