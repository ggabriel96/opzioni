#ifndef OPZIONI_EXTRA_HPP
#define OPZIONI_EXTRA_HPP

#include <string_view>
#include <tuple>
#include <vector>

namespace opz {

struct ExtraInfo {
  std::vector<std::string_view> parent_cmds_names;
};


template <typename T, typename... Ts>
void set_at(std::tuple<Ts...> &tuple, T const &value, std::size_t tgt_idx) {
  std::size_t idx = 0;
  // clang-format off
  std::apply(
    [&idx, tgt_idx, &value](auto&&... arg) {
      (void)(( // cast to void to suppress unused warning
      idx == tgt_idx
        ? (arg = value, true)
        : (++idx, false)
      ) || ...);
    },
    tuple
  );
  // clang-format on
}

} // namespace opz

#endif // OPZIONI_EXTRA_HPP