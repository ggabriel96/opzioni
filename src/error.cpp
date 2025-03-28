#include "opzioni/cmd.hpp"
#include "opzioni/exceptions.hpp"
#include "opzioni/strings.hpp"

#include <cstdio>

namespace opz {

int print_error(UserError &ue) noexcept {
  auto const info = ue.info.get();
  auto const msg = limit_string_within(ue.what(), info.msg_width);
  std::fputs(msg.c_str(), stderr);
  return -1;
}

int print_error_and_usage(UserError &ue) noexcept {
  print_error(ue);
  std::fputc(nl, stderr);
  auto const info = ue.info.get();
  info.print_usage(stderr);
  return -1;
}

int rethrow(UserError &ue) { throw ue; }

} // namespace opz
