#include <print>

#include "opzioni/cmd.hpp"

struct double_action {};

template <int TupleIdx, opz::concepts::Cmd Cmd>
void consume_arg(
  opz::ArgsMap<Cmd const> &args_map, opz::Arg<int, double_action> const &arg, opz::act::ArgValue const &value, Cmd const &, opz::ExtraInfo const &
) {
  if (value.index() != opz::act::pos_idx) throw "double_action can only be used with positionals";
  auto const val = opz::convert<int>(std::get<opz::act::pos_idx>(value));
  std::get<TupleIdx>(args_map.args) = 2 * val;
}

int main(int argc, char const *argv[]) {
  auto twice_cmd = opz::new_cmd("twice", "1.0")
                     .intro("Boring example of a custom action that doubles a number")
                     .pos<"number", int, double_action>({.help = "Just an integer"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = twice_cmd(argc, argv);
  auto const number = map.get<"number">();
  std::print("Doubled number: {}\n", number);
}
