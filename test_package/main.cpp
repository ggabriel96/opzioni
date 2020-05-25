#include <string>
#include <iostream>

#include <opzioni.hpp>

int main(int argc, char const *argv[]) {
  opz::ArgParser ap;
  ap.add(opz::Arg<std::string>("name").help("Your name"));

  auto const args = ap.parse_args(argc, argv);
  std::cout << "Number of arguments: " << args.size() << '\n';
  std::cout << "name: " << args["name"].as<std::string>() << '\n';
}
