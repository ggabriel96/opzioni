#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <catch2/catch.hpp>

#include "converters.hpp"

SCENARIO("provided converter for vector<int>", "[vector]") {
  using vec = std::vector<int>;

  GIVEN("a proper comma-separated list of integers") {
    auto const input = std::optional(std::string("1,2,3"));

    WHEN("converting the list") {
      auto const result = opzioni::convert<vec>(input);
      REQUIRE(result == vec{1, 2, 3});
    }
  }

  AND_GIVEN("a comma-separated list of integers with a trailing comma") {
    auto const input = std::optional(std::string("1,2,3,"));

    WHEN("converting the list") {
      auto const result = opzioni::convert<vec>(input);
      REQUIRE(result == vec{1, 2, 3});
    }
  }
}
