#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using opz::Arg;

  fmt::print("argv: {}\n", fmt::join(std::vector(argv, argv + argc), ", "));

  opz::ArgParser ap;
  ap.add(Arg<std::string>("name").help("Your name"));
  ap.add(Arg<std::string>("--last-name").help("Your last name"));
  ap.add(Arg<int>("-v").help("Level of verbosity").among({0, 1, 2, 3, 4}).with_default(0));
  ap.add(Arg<double>("-d").help("A double"));
  ap.add(Arg<int>("--flag").help("Long flag").with_default(7).as_flag(11));
  ap.add(Arg<bool>("-a").help("Short flag a").with_default(false).as_flag(true));
  ap.add(Arg<bool>("-b").help("Short flag b").as_flag());
  ap.add(Arg<std::vector<int>>("--numbers").help("A list of numbers"));

  auto const args = ap.parse(argc, argv);
  fmt::print("\nNumber of arguments: {}\n", args.size());
  fmt::print("name: {}\n", args["name"].as<std::string>());
  fmt::print("last name: {}\n", args["last-name"].as<std::string>());
  fmt::print("v: {}\n", args["v"].as<int>());
  fmt::print("d: {}\n", args["d"].as<double>());
  fmt::print("flag: {}\n", args["flag"].as<int>());
  fmt::print("a: {}\n", args["a"].as<bool>());
  fmt::print("b: {}\n", args["b"].as<bool>());
  fmt::print("numbers: [{}]\n", fmt::join(args["numbers"].as<std::vector<int>>(), ", "));
  if (args.remaining_args != nullptr) {
    fmt::print("remaining_args:\n");
    fmt::print("- size: {}\n", args.remaining_args->size());
    fmt::print("- capacity: {}\n", args.remaining_args->capacity());
    fmt::print("- items: [{}]\n", fmt::join(*args.remaining_args, ", "));
  }
}
