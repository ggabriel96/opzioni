#ifndef OPZIONI_VARIANT_HPP
#define OPZIONI_VARIANT_HPP

namespace opz {

struct empty {};

template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

} // namespace opz

#endif // OPZIONI_VARIANT_HPP
