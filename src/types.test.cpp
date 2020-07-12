#include <any>
#include <string>

#include <catch2/catch.hpp>

#include "types.hpp"

SCENARIO("ArgValue implicitly converts to T", "[user]") {

  GIVEN("an int") {
    opzioni::ArgValue const av{1};

    WHEN("assigning to an int") {
      int i = av;

      REQUIRE(i == std::any_cast<int>(av.value));
    }
  }

  GIVEN("a std::string") {
    opzioni::ArgValue const av{std::string("string")};

    WHEN("assigning to a std::string") {
      std::string s = av;

      REQUIRE(s == std::any_cast<std::string>(av.value));
    }
  }
}
