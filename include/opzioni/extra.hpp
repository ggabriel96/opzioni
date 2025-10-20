#ifndef OPZIONI_EXTRA_HPP
#define OPZIONI_EXTRA_HPP

#include <string_view>
#include <vector>

namespace opz {

struct ExtraInfo {
  std::vector<std::string_view> parent_cmds_names;
};

} // namespace opz

#endif // OPZIONI_EXTRA_HPP