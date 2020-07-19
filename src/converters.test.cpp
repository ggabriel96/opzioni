#include <optional>
#include <string>
#include <vector>

#include <catch2/catch.hpp>

#include "converters.hpp"

SCENARIO("provided converter for vector<int>", "[vector]") {
  using vec = std::vector<int>;
  using namespace std::string_literals;

  GIVEN("a proper comma-separated list of integers") {
    auto const input = "1,2,3"s;

    WHEN("converting the list") {
      auto const result = opzioni::convert<vec>(input);
      REQUIRE(result == vec{1, 2, 3});
    }
  }

  GIVEN("a comma-separated list of integers with a trailing comma") {
    auto const input = "1,2,3,"s;

    WHEN("converting the list") {
      auto const result = opzioni::convert<vec>(input);
      REQUIRE(result == vec{1, 2, 3});
    }
  }
}
