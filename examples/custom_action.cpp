#include <print>

#include "opzioni/cmd.hpp"

struct double_action {};

template <opz::concepts::Cmd Cmd>
void process(
  opz::ArgsMap<Cmd const> &args_map,
  opz::Arg<int, double_action> const &arg,
  std::optional<std::string_view> const &value,
  Cmd const &,
  opz::ExtraInfo const &
) {
  auto const val = opz::convert<int>(*value);
  auto const [_, inserted] = args_map.args.try_emplace(arg.name, 2 * val);
  if (!inserted) throw opz::UnexpectedValue(arg.name, 1, 2);
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
