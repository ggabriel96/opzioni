#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "converters.hpp"

namespace opzioni {

TEST(Converters, VectorOfInt) {
  auto const input = std::optional(std::string("1,2"));
  auto const expected = std::vector({1, 2});

  auto const result = convert<std::remove_cv_t<decltype(expected)>>(input);

  ASSERT_EQ(result, expected);
}

TEST(Converters, VectorOfIntWithTrailingComma) {
  auto const input = std::optional(std::string("1,"));
  auto const expected = std::vector({1});

  auto const result = convert<std::remove_cv_t<decltype(expected)>>(input);

  ASSERT_EQ(result, expected);
}

} // namespace opzioni
