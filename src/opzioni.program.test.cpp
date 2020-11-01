#include "opzioni.hpp"

#include <array>

#include <catch2/catch.hpp>

SCENARIO("argument specification") {
  using namespace opzioni;
  using namespace std::string_literals;
  using Catch::Matchers::Message;

  GIVEN("an empty program specification") {
    Program program;

    WHEN("user specifies a positional argument") {
      auto const positional_name = "positional"s;
      auto const &inserted_positional = program.pos(positional_name);

      THEN("it is added to the specification, is marked required, and its address is returned") {
        REQUIRE(program.positionals.size() == 1);
        REQUIRE(program.options.size() == 0);
        REQUIRE(program.flags.size() == 0);
        REQUIRE(&inserted_positional == &program.positionals[0]);

        REQUIRE(inserted_positional.name == positional_name);
        REQUIRE(inserted_positional.description == ""s);
        REQUIRE(inserted_positional.is_required);
        REQUIRE(inserted_positional.default_value == std::nullopt);
        REQUIRE(inserted_positional.set_value == std::monostate{});
        REQUIRE(inserted_positional.act ==
                static_cast<actions::signature<ArgumentType::POSITIONAL>>(actions::assign<std::string>));
        REQUIRE(inserted_positional.gather_n.amount == 1);
      }
    }

    WHEN("user specifies an option") {
      auto const option_name = "option"s;
      auto const &inserted_option = program.opt(option_name);

      THEN("it is added to the specification and its address is returned") {
        REQUIRE(program.positionals.size() == 0);
        REQUIRE(program.options.size() == 1);
        REQUIRE(program.flags.size() == 0);
        REQUIRE(&inserted_option == &program.options[option_name]);

        REQUIRE(inserted_option.name == option_name);
        REQUIRE(inserted_option.description == ""s);
        REQUIRE(!inserted_option.is_required);
        REQUIRE(inserted_option.default_value == std::nullopt);
        REQUIRE(inserted_option.set_value == std::nullopt);
        REQUIRE(inserted_option.act ==
                static_cast<actions::signature<ArgumentType::OPTION>>(actions::assign<std::string>));
        REQUIRE(inserted_option.gather_n.amount == 1);
      }
    }

    WHEN("user specifies a flag") {
      auto const flag_name = "flag"s;
      auto const &inserted_flag = program.flag(flag_name);

      THEN("it is added to the specification, set_value is set to true, action is set to assign<bool>, and its "
           "address is returned") {
        REQUIRE(program.positionals.size() == 0);
        REQUIRE(program.options.size() == 0);
        REQUIRE(program.flags.size() == 1);
        REQUIRE(&inserted_flag == &program.flags[flag_name]);

        REQUIRE(inserted_flag.name == flag_name);
        REQUIRE(inserted_flag.description == ""s);
        REQUIRE(!inserted_flag.is_required);
        REQUIRE(inserted_flag.default_value == std::nullopt);
        REQUIRE(inserted_flag.set_value.has_value());
        REQUIRE(std::holds_alternative<bool>(*inserted_flag.set_value));
        REQUIRE(std::get<bool>(inserted_flag.set_value.value()) == true);
        REQUIRE(inserted_flag.act == static_cast<actions::signature<ArgumentType::FLAG>>(actions::assign<bool>));
        REQUIRE(inserted_flag.gather_n == std::monostate{});
      }
    }
  }
}

SCENARIO("positional arguments") {
  using Catch::Matchers::Message;
  using namespace std::string_literals;

  opzioni::Program program;

  GIVEN("no arguments are specified for the program") {
    auto const cmd_name = "./test";

    WHEN("no arguments are given in CLI") {
      std::array argv{cmd_name};

      THEN("we set cmd_name, but parse nothing") {
        auto const result = program(argv);

        REQUIRE(result.args.empty());
        REQUIRE(result.cmd_name == cmd_name);
        REQUIRE(result.subcmd == nullptr);
      }
    }

    WHEN("one positional argument is given in CLI") {
      std::array argv{cmd_name, "positional"};

      THEN("we throw an error about that positional") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one dash is given in CLI") {
      std::array argv{cmd_name, "-"};

      THEN("we throw an error about that positional (the dash)") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `-`. This program expects 0 positional arguments"));
      }
    }

    WHEN("many positional arguments are given in CLI") {
      std::array argv{cmd_name, "positional", "sometime", "somewhere"};

      THEN("we throw an error about the first argument") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }

    WHEN("dash-dash followed by a positional argument are given the in CLI") {
      std::array argv{cmd_name, "--", "positional"};

      THEN("we throw an error for the argument after dash-dash") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }
  }

  GIVEN("1 command is specified for the program") {
    auto const cmd_name = "./test";
    auto const subcmd_name = "cmd"s;
    auto &subcmd = program.cmd(subcmd_name);

    WHEN("the command is given in CLI") {
      std::array argv{cmd_name, subcmd_name.c_str()};

      THEN("it is parsed as command; both cmd_name are set; everything else is empty") {
        auto const result = program(argv);

        REQUIRE(result.subcmd != nullptr);
        REQUIRE(result.subcmd->cmd_name == subcmd_name);
        REQUIRE(result.subcmd->args.empty());
        REQUIRE(result.subcmd->subcmd == nullptr);

        REQUIRE(result.cmd_name == cmd_name);
        REQUIRE(result.args.empty());
      }
    }

    WHEN("one positional argument is given in CLI") {
      std::array argv{cmd_name, "positional"};

      THEN("we throw an error about that positional") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }

    WHEN("dash-dash followed by the command are given the in CLI") {
      std::array argv{cmd_name, "--", subcmd_name.c_str()};

      THEN("we throw an error because the command was interpreted as positional") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `cmd`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one positional argument is given before the command in CLI") {
      std::array argv{cmd_name, "positional", "cmd"};

      THEN("we throw an error about that positional") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }

    WHEN("one positional argument is given after the command in CLI") {
      std::array argv{cmd_name, "cmd", "positional"};

      THEN("we throw an error about that positional") {
        REQUIRE_THROWS_MATCHES(
            program(argv), opzioni::UnknownArgument,
            Message("Unexpected positional argument `positional`. This program expects 0 positional arguments"));
      }
    }

    GIVEN("1 positional argument is specified for the command") {
      auto const pos = "pos"s;
      subcmd.pos(pos);

      WHEN("one positional argument is given after the command in CLI") {
        auto const pos_input = "positional"s;
        std::array argv{cmd_name, subcmd_name.c_str(), pos_input.c_str()};

        THEN("we parse the positional as an argument to the command") {
          auto const result = program(argv);

          REQUIRE(result.subcmd != nullptr);
          REQUIRE(result.subcmd->cmd_name == subcmd_name);
          REQUIRE(result.subcmd->subcmd == nullptr);
          REQUIRE(result.subcmd->args.size() == 1);
          REQUIRE(result.subcmd->as<std::string>(pos) == pos_input);

          REQUIRE(result.cmd_name == cmd_name);
          REQUIRE(result.args.empty());
        }
      }
    }
  }
}
