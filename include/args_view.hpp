#ifndef OPZIONI_ARGS_VIEW_H
#define OPZIONI_ARGS_VIEW_H

#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace opz {

struct ArgsView {
  std::string_view exec_path{};
  // TODO: put positionals and options on the same map when we start querying the command args?
  std::vector<std::string_view> positionals;
  std::map<std::string_view, std::optional<std::string_view>> options;
  std::unique_ptr<ArgsView> sub_cmd{}; // unique_ptr because ArgsView is still not a complete type at this line

  void print_debug() const noexcept;
};

} // namespace opz

#endif // OPZIONI_ARGS_VIEW_H
