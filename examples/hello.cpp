#include <iostream>
#include <string_view>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using namespace opzioni;

  constexpr auto hello = Program("hello")
                             .version("0.1")
                             .intro("Greeting people since the dawn of computing")
                             .add(Pos("name").help("Your name please, so I can greet you"))
                             .add(Help())
                             .add(Version());

  auto const args = hello(argc, argv);
  std::string_view const name = args["name"];
  std::cout << "Hello, " << name << "!\n";
}
