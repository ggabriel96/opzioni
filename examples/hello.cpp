#include <print>

#include "opzioni/all.hpp"

int main(int argc, char const *argv[]) {
  auto hello_cmd = opz::new_cmd("hello", "1.0")
                     .intro("Greeting people since the dawn of computing")
                     .pos<"name">({.help = "Your name please, so I can greet you"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = hello_cmd(argc, argv);
  auto const name = map.get<"name">();
  std::print("Hello, {}!\n", name);
}
