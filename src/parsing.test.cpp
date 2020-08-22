#include "parsing.hpp"

#include <array>
#include <string>

#include <catch2/catch.hpp>

SCENARIO("positional arguments") {
  using namespace std::string_literals;
  opzioni::Program program;

  GIVEN("no arguments are specified for the program") {

    WHEN("no arguments are given in CLI") {
      std::array argv{"./test"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we parse nothing") {
        auto const result = parser();

        REQUIRE(result.args.empty());

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("one positional argument is given in CLI") {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("one dash is given in CLI") {
      std::array argv{"./test", "-"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("many positional arguments are given in CLI") {
      std::array argv{"./test", "someone", "sometime", "somewhere"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("dash-dash followed by a positional argument are given the in CLI") {
      std::array argv{"./test", "--", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("dash-dash followed by what would be a flag are given the in CLI") {
      std::array argv{"./test", "--", "--flag"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("dash-dash followed by what would be a short flag are given the in CLI") {
      std::array argv{"./test", "--", "-f"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }
  }

  GIVEN("1 command is specified for the program") {
    auto const cmd = "cmd"s;
    program.cmd(cmd);

    WHEN("one positional argument is given in CLI") {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("the command is given in CLI") {
      std::array argv{"./test", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("it is parsed as command") {
        auto const result = parser();

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == "cmd"s);
        REQUIRE(result.subcmd->args.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.args.empty());
      }
    }

    WHEN("dash-dash followed by the command are given the in CLI") {
      std::array argv{"./test", "--", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("one positional argument is given before the command in CLI") {
      std::array argv{"./test", "someone", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }

    WHEN("one positional argument is given after the command in CLI") {
      std::array argv{"./test", "cmd", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") { REQUIRE_THROWS_AS(parser(), opzioni::UnknownArgument); }
    }
  }
}
