#include <array>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("adding arguments", "[Program]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("an empty Program") {
    Program program;

    REQUIRE(program.args().size() == 0);
    REQUIRE(program.positionals_amount == 0);

    WHEN("a positional is added") {
      program + std::array{Pos("pos")};

      REQUIRE(program.args().size() == 1);
      REQUIRE(program.positionals_amount == 1);
      REQUIRE(program.args()[0].name == "pos");
    }

    WHEN("two positionals are added") {
      program + Pos("pos1") * Pos("pos2");

      REQUIRE(program.args().size() == 2);
      REQUIRE(program.positionals_amount == 2);
      REQUIRE(program.args()[0].name == "pos1");
      REQUIRE(program.args()[1].name == "pos2");
    }

    WHEN("multiple positionals are added") {
      program + Pos("pos5") * Pos("pos2") * Pos("pos4") * Pos("pos1") * Pos("pos3");

      REQUIRE(program.args().size() == 5);
      REQUIRE(program.positionals_amount == 5);
      REQUIRE(program.args()[0].name == "pos5");
      REQUIRE(program.args()[1].name == "pos2");
      REQUIRE(program.args()[2].name == "pos4");
      REQUIRE(program.args()[3].name == "pos1");
      REQUIRE(program.args()[4].name == "pos3");
    }
  }
}
