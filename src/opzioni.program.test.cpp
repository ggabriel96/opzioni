#include <algorithm>
#include <array>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("adding arguments", "[Program]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("an empty Program") {
    Program program;

    THEN("it should initially have no arguments") {
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.positionals_amount == 0);
    }

    WHEN("a positional is added") {
      program + std::array{Pos("pos")};

      REQUIRE(program.args().size() == 1);

      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 1); }
      THEN("it should be added as first argument") { REQUIRE(program.args()[0].name == "pos"); }
    }

    WHEN("two positionals are added") {
      program + Pos("pos1") * Pos("pos2");

      REQUIRE(program.args().size() == 2);
      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 2); }
      THEN("they should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos1");
        REQUIRE(program.args()[1].name == "pos2");
      }
    }

    WHEN("multiple positionals are added") {
      program + Pos("pos5") * Pos("pos2") * Pos("pos4") * Pos("pos1") * Pos("pos3");

      REQUIRE(program.args().size() == 5);
      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 5); }
      THEN("they should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos5");
        REQUIRE(program.args()[1].name == "pos2");
        REQUIRE(program.args()[2].name == "pos4");
        REQUIRE(program.args()[3].name == "pos1");
        REQUIRE(program.args()[4].name == "pos3");
      }
    }

    WHEN("other arguments are added before positionals") {
      program + Flg("flg1") * Opt("opt2") * Pos("pos3") * Pos("pos1") * Pos("pos2") * Opt("opt1") * Flg("flg2");

      REQUIRE(program.args().size() == 7);
      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 3); }
      THEN("positionals should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos3");
        REQUIRE(program.args()[1].name == "pos1");
        REQUIRE(program.args()[2].name == "pos2");
      }
      THEN("the rest of the arguments should be sorted lexicographically by name") {
        // also using is_sorted to check `operator<`
        REQUIRE(std::is_sorted(std::next(program.args().begin(), program.positionals_amount), program.args().end()));
        REQUIRE(program.args()[3].name == "flg1");
        REQUIRE(program.args()[4].name == "flg2");
        REQUIRE(program.args()[5].name == "opt1");
        REQUIRE(program.args()[6].name == "opt2");
      }
    }
  }
}
