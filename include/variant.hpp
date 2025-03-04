#ifndef OPZIONI_VARIANT_H
#define OPZIONI_VARIANT_H

namespace opz {

template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

} // namespace opz

#endif // OPZIONI_VARIANT_H
