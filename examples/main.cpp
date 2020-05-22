#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "opzioni.hpp"

constexpr char nl = '\n';

int main(int argc, char const *argv[]) {
  using opz::Arg;

  std::cout << "argv:\n";
  std::for_each(argv, argv + argc,
                [](char const *arg) { std::cout << arg << nl; });
  std::cout << nl;

  opz::ArgParser ap;
  ap.add(Arg<std::string>("name").help("Your name"));
  ap.add(Arg<std::string>("--last-name").help("Your last name"));
  ap.add(Arg<int>("-v")
             .help("Level of verbosity")
             .among({0, 1, 2, 3, 4})
             .with_default(0));
  ap.add(Arg<int>("--flag").help("Long flag").with_default(7).as_flag(11));
  ap.add(
      Arg<bool>("-a").help("Short flag a").with_default(false).as_flag(true));
  ap.add(Arg<bool>("-b").help("Short flag b").as_flag());
  ap.add(Arg<std::vector<int>>("--numbers").help("A list of numbers"));

  auto const args = ap.parse_args(argc, argv);
  std::cout << std::boolalpha;
  std::cout << "\nNumber of arguments: " << args.size() << nl;
  std::cout << "name: " << args["name"].as<std::string>() << nl;
  std::cout << "last name: " << args["last-name"].as<std::string>() << nl;
  std::cout << "v: " << args["v"].as<int>() << nl;
  std::cout << "flag: " << args["flag"].as<int>() << nl;
  std::cout << "a: " << args["a"].as<bool>() << nl;
  std::cout << "b: " << args["b"].as<bool>() << nl;
  auto const numbers = args["numbers"].as<std::vector<int>>();
  std::cout << "numbers:\n";
  std::for_each(begin(numbers), end(numbers),
                [](auto const &elem) { std::cout << "- " << elem << nl; });
  if (args.remaining_args != nullptr) {
    std::cout << "remaining_args:\n";
    std::cout << "- size: " << args.remaining_args->size() << nl;
    std::cout << "- capacity: " << args.remaining_args->capacity() << nl;
    std::for_each(args.remaining_args->begin(), args.remaining_args->end(),
                  [](auto const &elem) { std::cout << "- " << elem << nl; });
  }
}
