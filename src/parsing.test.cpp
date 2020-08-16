#include "parsing.hpp"

#include <array>
#include <string>

#include <catch2/catch.hpp>

SCENARIO("positional arguments") {
  using namespace std::string_literals;
  opzioni::Program program;

  GIVEN("no positional arguments are specified for the program") {

    WHEN("no arguments are given in CLI") {
      std::array argv{"./test"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("we parse nothing") {
        auto const result = parser();

        REQUIRE(result.positional.empty());

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("one positional argument is given in CLI") {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("it is parsed as positional") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "someone"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("many positional arguments are given in CLI") {
      std::array argv{"./test", "someone", "sometime", "somewhere"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("they are all parsed as positional") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 2);
        REQUIRE(result.positional[0] == "someone"s);
        REQUIRE(result.positional[1] == "sometime"s);
        REQUIRE(result.positional[2] == "somewhere"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("dash-dash followed by a positional argument are given the in CLI") {
      std::array argv{"./test", "--", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("the argument is parsed as positional") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "someone"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("dash-dash followed by what would be a flag are given the in CLI") {
      std::array argv{"./test", "--", "--flag"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("the supposed flag is parsed as a positional argument") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "--flag"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("dash-dash followed by what would be a short flag are given the in CLI") {
      std::array argv{"./test", "--", "-f"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("the supposed flag is parsed as a positional argument") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "-f"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }
  }

  GIVEN("1 positional argument and 1 subcommand") {
    program.pos("name");
    program.cmd("cmd");

    WHEN("only the positional is in CLI") {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("it is parsed as positional") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "someone"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("only the subcommand is in CLI") {
      std::array argv{"./test", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("it is parsed as subcommand") {
        auto const result = parser();

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == "cmd"s);
        REQUIRE(result.subcmd->flags.empty());
        REQUIRE(result.subcmd->options.empty());
        REQUIRE(result.subcmd->positional.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
        REQUIRE(result.positional.empty());
      }
    }

    WHEN("both are in CLI, but positional comes first") {
      std::array argv{"./test", "name", "cmd"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("both are parsed as arguments to the program") {
        auto const result = parser();

        REQUIRE(result.positional.size() == 1);
        REQUIRE(result.positional[0] == "name"s);

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == "cmd"s);
        REQUIRE(result.subcmd->flags.empty());
        REQUIRE(result.subcmd->options.empty());
        REQUIRE(result.subcmd->positional.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
      }
    }

    WHEN("both are in CLI, but subcommand comes first") {
      std::array argv{"./test", "cmd", "name"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN("the subcommand is parsed as argument to the program and the positional as argument of the subcommand") {
        auto const result = parser();

        REQUIRE(result.positional.empty());

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == "cmd"s);
        REQUIRE(result.subcmd->flags.empty());
        REQUIRE(result.subcmd->options.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);
        REQUIRE(result.subcmd->positional.size() == 1);
        REQUIRE(result.subcmd->positional[0] == "name"s);

        REQUIRE(result.cmd_name == "./test");
        REQUIRE(result.flags.empty());
        REQUIRE(result.options.empty());
      }
    }
  }
}
