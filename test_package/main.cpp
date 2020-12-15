#include <iostream>
#include <string_view>

#include <opzioni.hpp>

int main(int argc, char const *argv[]) {
  using opzioni::Program, opzioni::Pos;

  auto program = Program("test-package") + Pos("name").help("Your name");

  auto const args = program(argc, argv);
  std::cout << "Number of arguments: " << args.size() << '\n';
  std::cout << "name: " << args["name"].as<std::string_view>() << '\n';
}
