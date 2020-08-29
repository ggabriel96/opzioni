#include "parsing.hpp"

#include <array>
#include <string>

#include <catch2/catch.hpp>

SCENARIO("positional arguments") {
  using Catch::Matchers::Message;
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

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one dash is given in CLI") {
      std::array argv{"./test", "-"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `-`. This program expects 0 positional arguments"));
      }
    }

    WHEN("many positional arguments are given in CLI") {
      std::array argv{"./test", "someone", "sometime", "somewhere"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }

    WHEN("dash-dash followed by a positional argument are given the in CLI") {
      std::array argv{"./test", "--", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }
  }

  GIVEN("1 command is specified for the program") {
    auto const cmd = "cmd"s;
    auto &subcmd = program.cmd(cmd);

    WHEN("one positional argument is given in CLI") {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }

    WHEN("the command is given in CLI") {
      std::array argv{"./test", cmd.c_str()};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("it is parsed as command") {
        auto const result = parser();

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == cmd);
        REQUIRE(result.subcmd->args.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.args.empty());
      }
    }

    WHEN("dash-dash followed by the command are given the in CLI") {
      std::array argv{"./test", "--", cmd.c_str()};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `cmd`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one positional argument is given before the command in CLI") {
      std::array argv{"./test", "someone", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one positional argument is given after the command in CLI") {
      std::array argv{"./test", "cmd", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we throw an error") {
        REQUIRE_THROWS_MATCHES(
            parser(), opzioni::UnknownArgument,
            Message("Unexpected positional argument `someone`. This program expects 0 positional arguments"));
      }
    }

    GIVEN("1 positional argument is specified for the command") {
      auto const pos = "pos"s;
      subcmd.pos(pos);

      WHEN("one positional argument is given after the command in CLI") {
        auto const pos_input = "someone"s;
        std::array argv{"./test", cmd.c_str(), pos_input.c_str()};
        opzioni::parsing::ArgumentParser parser(program, argv);

        THEN("we parse the positional as an argument to the command") {
          auto const result = parser();

          REQUIRE(result.subcmd != nullptr);
          REQUIRE(result.subcmd->cmd_name == cmd);
          REQUIRE(result.subcmd->subcmd == nullptr);
          REQUIRE(result.subcmd->args.size() == 1);
          REQUIRE(result.subcmd->as<std::string>(pos) == pos_input);

          REQUIRE(result.cmd_name == "./test");
          REQUIRE(result.args.empty());
        }
      }
    }
  }
}
