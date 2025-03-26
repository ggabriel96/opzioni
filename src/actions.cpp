#include "actions.hpp"
#include "cmd_info.hpp"

#include <iostream>

namespace opz::act {

void print_help(
  std::map<std::string_view, std::any> &, Arg<bool> const &, std::optional<std::string_view> const,
  CmdInfoGetter &info) {
  auto const &formatter = info.get();
  formatter.print_title();
  if (!formatter.introduction.empty()) {
    std::cout << nl;
    formatter.print_intro();
  }
  std::cout << nl;
  formatter.print_usage();
  std::cout << nl;
  formatter.print_help();
  std::cout << nl;
  formatter.print_details();
  std::exit(0);
}

void print_version(
  std::map<std::string_view, std::any> &, Arg<bool> const &, std::optional<std::string_view> const,
  CmdInfoGetter &info) {
  auto const &formatter = info.get();
  fmt::print("{} {}\n", formatter.name, formatter.version);
  std::exit(0);
}

} // namespace opz::act
