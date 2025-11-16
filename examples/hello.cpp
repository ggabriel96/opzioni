#include <print>

#include "opzioni/all.hpp"

int main(int argc, char const *argv[]) {
  auto hello_cmd = opz::new_cmd("hello", "1.0")
                     .intro("Greeting people since the dawn of computing")
                     .pos<"pos1">({.help = "Positional 1"})
                     .pos<"pos2">({.help = "Positional 2"})
                     .opt<"opt1", "O">({.help = "Option 1"})
                     .flg<"flg1", "f">({.help = "Flag 1"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = hello_cmd(argc, argv);
  auto const pos1 = *std::get<0>(map.t_args);
  auto const pos2 = *std::get<1>(map.t_args);
  auto const opt1 = *std::get<2>(map.t_args);
  auto const flg1 = *std::get<3>(map.t_args);
  std::print("pos1=`{}`\n", pos1);
  std::print("pos2=`{}`\n", pos2);
  std::print("opt1=`{}`\n", opt1);
  std::print("flg1=`{}`\n", flg1);
}
