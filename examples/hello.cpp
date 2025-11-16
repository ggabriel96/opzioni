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
  auto const pos1 = map.get<"pos1">();
  auto const pos2 = map.get<"pos2">();
  auto const opt1 = map.get<"opt1">();
  auto const flg1 = map.get<"flg1">();
  fmt::print("pos1=`{}`\n", pos1);
  fmt::print("pos2=`{}`\n", pos2);
  fmt::print("opt1=`{}`\n", opt1);
  fmt::print("flg1=`{}`\n", flg1);
}
