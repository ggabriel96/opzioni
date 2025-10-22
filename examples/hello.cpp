#include <print>

#include "opzioni/all.hpp"

int main(int argc, char const *argv[]) {
  auto hello_cmd = opz::new_cmd("hello", "1.0")
                     .intro("Greeting people since the dawn of computing")
                     .pos<"a">({.help = "Param a"})
                     .pos<"b">({.help = "Param b"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = hello_cmd(argc, argv);
  auto const a = *std::get<0>(map.t_args);
  auto const b = *std::get<1>(map.t_args);
  std::print("a={}, b={}\n", a, b);
}
