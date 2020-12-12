#include <optional>
#include <string_view>
#include <vector>

#include <catch2/catch.hpp>

#include "converters.hpp"

SCENARIO("bool", "[BuiltinType]") {
  using type = bool;
  using namespace std::string_view_literals;

  GIVEN("'0' as input string") {
    auto const input = "0"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<type>(input);

      THEN("we should get false") { REQUIRE(result == false); }
    }
  }

  GIVEN("'1' as input string") {
    auto const input = "1"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<type>(input);

      THEN("we should get true") { REQUIRE(result == true); }
    }
  }

  GIVEN("'false' as input string") {
    auto const input = "false"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<type>(input);

      THEN("we should get false") { REQUIRE(result == false); }
    }
  }

  GIVEN("'true' as input string") {
    auto const input = "true"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<type>(input);

      THEN("we should get true") { REQUIRE(result == true); }
    }
  }

  GIVEN("an empty string") {
    auto const input = ""sv;

    WHEN("calling convert") {
      THEN("we should throw an error") { REQUIRE_THROWS_AS(opzioni::convert<type>(input), opzioni::ConversionError); }
    }
  }
}

SCENARIO("vector<int>", "[vector]") {
  using vec = std::vector<int>;
  using namespace std::string_view_literals;

  GIVEN("an empty string") {
    auto const input = ""sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<vec>(input);

      THEN("we should get an empty vector") { REQUIRE(result == vec{}); }
    }
  }

  GIVEN("a list of only one integer") {
    auto const input = "2"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<vec>(input);

      THEN("we should get a vector of int of one element") { REQUIRE(result == vec{2}); }
    }
  }

  GIVEN("a proper comma-separated list of integers") {
    auto const input = "1,2,3"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<vec>(input);

      THEN("we should get a vector of int of that list") { REQUIRE(result == vec{1, 2, 3}); }
    }
  }

  GIVEN("a comma-separated list of integers with a trailing comma") {
    auto const input = "1,2,3,"sv;

    WHEN("calling convert") {
      auto const result = opzioni::convert<vec>(input);

      THEN("we should get a vector of int of that list") { REQUIRE(result == vec{1, 2, 3}); }
    }
  }

  GIVEN("a comma-separated list of integers with a leading comma") {
    auto const input = ",1,2,3"sv;

    WHEN("calling convert") {
      THEN("we should throw an error") { REQUIRE_THROWS_AS(opzioni::convert<vec>(input), opzioni::ConversionError); }
    }
  }
}
