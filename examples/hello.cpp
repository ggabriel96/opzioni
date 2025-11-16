#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni/all.hpp"

int main(int argc, char const *argv[]) {
  auto hello_cmd = opz::new_cmd("hello", "1.0")
                     .intro("Greeting people since the dawn of computing")
                     .pos<"pos1">({.help = "Positional 1"})
                     .pos<"pos2">({.help = "Positional 2"})
                     .opt<"opt1", "O", std::vector<int>, opz::act::append>({.help = "Option 1"})
                     .flg<"flg1", "f">({.help = "Flag 1"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = hello_cmd(argc, argv);
  auto const pos1 = *std::get<0>(map.t_args);
  auto const pos2 = *std::get<1>(map.t_args);
  auto const opt1 = *std::get<2>(map.t_args);
  auto const flg1 = *std::get<3>(map.t_args);
  fmt::print("pos1=`{}`\n", pos1);
  fmt::print("pos2=`{}`\n", pos2);
  fmt::print("opt1=`{}`\n", opt1);
  fmt::print("flg1=`{}`\n", flg1);
}
