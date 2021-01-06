#include <algorithm>
#include <array>
#include <span>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("setting general information", "[Program][defaults][info][setters]") {
  using namespace opzioni;

  GIVEN("a default-initialized Program") {
    Program program;

    THEN("all member variables should have their defaults") {
      REQUIRE(program.metadata.name.empty());
      REQUIRE(program.metadata.version.empty());
      REQUIRE(program.metadata.title.empty());
      REQUIRE(program.metadata.introduction.empty());
      REQUIRE(program.metadata.description.empty());
      REQUIRE(program.metadata.msg_width == 100);
      REQUIRE(program.metadata.positionals_amount == 0);
      REQUIRE(program.metadata.error_handler == print_error);
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
    }

    WHEN("intro is called") {
      program.intro("intro");

      THEN("only the introduction should be changed") {
        REQUIRE(program.metadata.introduction == "intro");
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.version.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.description.empty());
        REQUIRE(program.metadata.msg_width == 100);
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.metadata.error_handler == print_error);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("details is called") {
      program.details("details");

      THEN("only the description should be changed") {
        REQUIRE(program.metadata.description == "details");
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.version.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.introduction.empty());
        REQUIRE(program.metadata.msg_width == 100);
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.metadata.error_handler == print_error);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("version is called") {
      program.version("1.0");

      THEN("only the version should be changed") {
        REQUIRE(program.metadata.version == "1.0");
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.introduction.empty());
        REQUIRE(program.metadata.description.empty());
        REQUIRE(program.metadata.msg_width == 100);
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.metadata.error_handler == print_error);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("intro, details, and version are called") {
      program.intro("intro").details("details").version("1.0");

      THEN("all three should be changed") {
        REQUIRE(program.metadata.introduction == "intro");
        REQUIRE(program.metadata.description == "details");
        REQUIRE(program.metadata.version == "1.0");
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.msg_width == 100);
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.metadata.error_handler == print_error);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("max_width is called") {
      program.max_width(80);

      THEN("only the msg_width should be changed") {
        REQUIRE(program.metadata.msg_width == 80);
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.version.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.introduction.empty());
        REQUIRE(program.metadata.description.empty());
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.metadata.error_handler == print_error);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("on_error is called") {
      program.on_error(nullptr);

      THEN("only the error_handler should be changed") {
        REQUIRE(program.metadata.error_handler == nullptr);
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.version.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.introduction.empty());
        REQUIRE(program.metadata.description.empty());
        REQUIRE(program.metadata.msg_width == 100);
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("intro, details, version, max_width, and on_error are called") {
      program.intro("intro").details("details").version("1.0").max_width(80).on_error(nullptr);

      THEN("all five should be changed") {
        REQUIRE(program.metadata.introduction == "intro");
        REQUIRE(program.metadata.description == "details");
        REQUIRE(program.metadata.version == "1.0");
        REQUIRE(program.metadata.msg_width == 80);
        REQUIRE(program.metadata.error_handler == nullptr);
        REQUIRE(program.metadata.name.empty());
        REQUIRE(program.metadata.title.empty());
        REQUIRE(program.metadata.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }
  }

  GIVEN("a Program initialized with a name") {
    Program const program("program");

    THEN("only the name should not have its default value") {
      REQUIRE(program.metadata.name == "program");
      REQUIRE(program.metadata.version.empty());
      REQUIRE(program.metadata.title.empty());
      REQUIRE(program.metadata.introduction.empty());
      REQUIRE(program.metadata.description.empty());
      REQUIRE(program.metadata.msg_width == 100);
      REQUIRE(program.metadata.positionals_amount == 0);
      REQUIRE(program.metadata.error_handler == print_error);
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
    }
  }

  GIVEN("a Program initialized with a name and title") {
    Program const program("program", "title");

    THEN("only name and title should not have their default values") {
      REQUIRE(program.metadata.name == "program");
      REQUIRE(program.metadata.title == "title");
      REQUIRE(program.metadata.version.empty());
      REQUIRE(program.metadata.introduction.empty());
      REQUIRE(program.metadata.description.empty());
      REQUIRE(program.metadata.msg_width == 100);
      REQUIRE(program.metadata.positionals_amount == 0);
      REQUIRE(program.metadata.error_handler == print_error);
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
    }
  }
}

SCENARIO("adding arguments", "[Program][args]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("an empty Program") {
    Program program;

    THEN("it should initially have no arguments") {
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
      REQUIRE(program.metadata.positionals_amount == 0);
    }

    WHEN("a positional is added") {
      program + std::array{Pos("pos")};

      THEN("the size of args should match the number of arguments added") { REQUIRE(program.args().size() == 1); }
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should match the number of positionals added") {
        REQUIRE(program.metadata.positionals_amount == 1);
      }
      THEN("the positional should be added as first argument") { REQUIRE(program.args()[0].name == "pos"); }
    }

    WHEN("two positionals are added") {
      program + Pos("pos1") * Pos("pos2");

      THEN("the size of args should match the number of arguments added") { REQUIRE(program.args().size() == 2); }
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should match the number of positionals added") {
        REQUIRE(program.metadata.positionals_amount == 2);
      }
      THEN("positionals should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos1");
        REQUIRE(program.args()[1].name == "pos2");
      }
    }

    WHEN("multiple positionals are added") {
      program + Pos("pos5") * Pos("pos2") * Pos("pos4") * Pos("pos1") * Pos("pos3");

      THEN("the size of args should match the number of arguments added") { REQUIRE(program.args().size() == 5); }
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should match the number of positionals added") {
        REQUIRE(program.metadata.positionals_amount == 5);
      }
      THEN("positionals should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos5");
        REQUIRE(program.args()[1].name == "pos2");
        REQUIRE(program.args()[2].name == "pos4");
        REQUIRE(program.args()[3].name == "pos1");
        REQUIRE(program.args()[4].name == "pos3");
      }
    }

    WHEN("other arguments are added before positionals") {
      program + Flg("flg1") * Opt("opt2") * Pos("pos3") * Pos("pos1") * Pos("pos2") * Opt("opt1") * Flg("flg2");

      THEN("the size of args should match the number of arguments added") { REQUIRE(program.args().size() == 7); }
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should match the number of positionals added") {
        REQUIRE(program.metadata.positionals_amount == 3);
      }
      THEN("positionals should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos3");
        REQUIRE(program.args()[1].name == "pos1");
        REQUIRE(program.args()[2].name == "pos2");
      }
      THEN("the rest of the arguments should be sorted lexicographically by name") {
        // also using is_sorted to check `operator<`
        REQUIRE(std::is_sorted(std::next(program.args().begin(), program.metadata.positionals_amount),
                               program.args().end()));
        REQUIRE(program.args()[3].name == "flg1");
        REQUIRE(program.args()[4].name == "flg2");
        REQUIRE(program.args()[5].name == "opt1");
        REQUIRE(program.args()[6].name == "opt2");
      }
    }
  }

  WHEN("adding arguments directly to a new Program") {
    auto const program =
        Program() + Flg("flg1") * Opt("opt2") * Pos("pos3") * Pos("pos1") * Pos("pos2") * Opt("opt1") * Flg("flg2");

    THEN("the size of args should match the number of arguments added") { REQUIRE(program.args().size() == 7); }
    THEN("cmds should be empty") { REQUIRE(program.cmds().size() == 0); }
    THEN("positionals_amount should match the number of positionals added") {
      REQUIRE(program.metadata.positionals_amount == 3);
    }
    THEN("positionals should be added as first arguments, but preserving the order of insertion") {
      REQUIRE(program.args()[0].name == "pos3");
      REQUIRE(program.args()[1].name == "pos1");
      REQUIRE(program.args()[2].name == "pos2");
    }
    THEN("the rest of the arguments should be sorted lexicographically by name") {
      // also using is_sorted to check `operator<`
      REQUIRE(
          std::is_sorted(std::next(program.args().begin(), program.metadata.positionals_amount), program.args().end()));
      REQUIRE(program.args()[3].name == "flg1");
      REQUIRE(program.args()[4].name == "flg2");
      REQUIRE(program.args()[5].name == "opt1");
      REQUIRE(program.args()[6].name == "opt2");
    }
  }
}

SCENARIO("adding commands", "[Program][cmds]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("program and cmd") {
    auto const cmd_name = "cmd"sv;
    Program const cmd(cmd_name);
    Program program;

    THEN("they should initially have no arguments") {
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
      REQUIRE(program.metadata.positionals_amount == 0);

      REQUIRE(cmd.args().size() == 0);
      REQUIRE(cmd.cmds().size() == 0);
      REQUIRE(cmd.metadata.positionals_amount == 0);
    }

    WHEN("cmd is added as command of program") {
      program + Cmd(cmd);

      THEN("cmd should be added as only command of program") {
        REQUIRE(program.cmds().size() == 1);
        REQUIRE(program.cmds()[0].program == &cmd);
      }
      THEN("program's args should not be changed") {
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.metadata.positionals_amount == 0);
      }

      AND_WHEN("another command with the same name is added to program") {
        Program other_cmd("cmd");

        THEN("we should thow an error because of duplicate name") {
          REQUIRE_THROWS_AS(program + Cmd(cmd) + Cmd(other_cmd), ArgumentAlreadyExists);
        }
      }

      AND_GIVEN("another command, cmd2") {
        auto const cmd2_name = "cmd2";
        Program const cmd2(cmd2_name);

        THEN("cmd2 should initially have no arguments") {
          REQUIRE(cmd2.args().size() == 0);
          REQUIRE(cmd2.cmds().size() == 0);
          REQUIRE(cmd2.metadata.positionals_amount == 0);
        }

        WHEN("cmd2 is added as command of program") {
          program + Cmd(cmd2);

          THEN("cmd2 should be added as second command of program") {
            REQUIRE(program.cmds().size() == 2);
            REQUIRE(program.cmds()[0].program == &cmd);
            REQUIRE(program.cmds()[1].program == &cmd2);
          }
          THEN("program's args should not be changed") {
            REQUIRE(program.args().size() == 0);
            REQUIRE(program.metadata.positionals_amount == 0);
          }
        }
      }
    }

    AND_WHEN("cmd is added as command of program twice") {
      THEN("we should thow an error because of duplicate name") {
        REQUIRE_THROWS_AS(program + Cmd(cmd) + Cmd(cmd), ArgumentAlreadyExists);
      }
    }
  }

  GIVEN("program, cmd1, and cmd2") {
    auto const cmd1_name = "cmd1"sv, cmd2_name = "cmd2"sv;
    Program const cmd1(cmd1_name);
    Program const cmd2(cmd2_name);
    Program program;

    THEN("they should initially have no arguments") {
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
      REQUIRE(program.metadata.positionals_amount == 0);

      REQUIRE(cmd1.args().size() == 0);
      REQUIRE(cmd1.cmds().size() == 0);
      REQUIRE(cmd1.metadata.positionals_amount == 0);

      REQUIRE(cmd2.args().size() == 0);
      REQUIRE(cmd2.cmds().size() == 0);
      REQUIRE(cmd2.metadata.positionals_amount == 0);
    }

    WHEN("both are added as commands of program, but cmd2 first") {
      program + Cmd(cmd2) + Cmd(cmd1);

      THEN("cmd2 should be added as first command of program and cmd1 as second") {
        REQUIRE(program.cmds().size() == 2);
        REQUIRE(program.cmds()[0].program == &cmd2);
        REQUIRE(program.cmds()[1].program == &cmd1);
      }
      THEN("program's args should not be changed") {
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.metadata.positionals_amount == 0);
      }
    }
  }
}

SCENARIO("parsing", "[Program][parsing]") {
  using namespace opzioni;

  GIVEN("an empty Program") {
    auto const program = Program().on_error(rethrow);

    WHEN("an empty argv is parsed") {
      std::array<char const *, 0> argv;

      auto const map = program(std::span(argv));

      THEN("nothing should be parsed") {
        REQUIRE(map.exec_path == "");
        REQUIRE(map.cmd_name == "");
        REQUIRE(map.cmd_args == nullptr);
        REQUIRE(map.args.size() == 0);
      }
    }

    AND_WHEN("an argv with only 1 element is parsed") {
      auto argv = std::array{"./program"};

      auto const map = program(std::span(argv));

      THEN("exec_path should be set") { REQUIRE(map.exec_path == std::string_view(argv[0])); }
      AND_THEN("nothing else should be parsed") {
        REQUIRE(map.cmd_name == "");
        REQUIRE(map.cmd_args == nullptr);
        REQUIRE(map.args.size() == 0);
      }
    }

    AND_WHEN("an argv with only dash-dash is parsed") {
      auto argv = std::array{"./program", "--"};

      auto const map = program(std::span(argv));

      THEN("exec_path should be set") { REQUIRE(map.exec_path == std::string_view(argv[0])); }
      AND_THEN("nothing else should be parsed") {
        REQUIRE(map.cmd_name == "");
        REQUIRE(map.cmd_args == nullptr);
        REQUIRE(map.args.size() == 0);
      }
    }

    AND_WHEN("an argv with only a single dash") {
      auto argv = std::array{"./program", "-"};

      THEN("we should throw an error because of unexpected positionals") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra positional arguments is parsed") {
      auto argv = std::array{"./program", "pos"};

      THEN("we should throw an error because of unexpected positionals") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra options is parsed (1)") {
      auto argv = std::array{"./program", "--opt=val"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra options is parsed (2)") {
      auto argv = std::array{"./program", "--opt", "val"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra options is parsed (3)") {
      auto argv = std::array{"./program", "-o=val"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra options is parsed (3)") {
      auto argv = std::array{"./program", "-o", "val"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra options is parsed (4)") {
      auto argv = std::array{"./program", "-oval"}; // also acts like multiple short flags

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra flags is parsed (1)") {
      auto argv = std::array{"./program", "--flag"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with extra flags is parsed (2)") {
      auto argv = std::array{"./program", "-f"};

      THEN("we should throw an error because of unknown arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }
  }

  GIVEN("a program with all kinds of arguments and a command") {
    constexpr auto cmd_args = Flg("f") * Pos("pos1") * Pos("cmd-pos") * Opt("long", "l");
    auto const cmd = Program("cmd").on_error(rethrow) + cmd_args;

    constexpr auto args = Pos("pos2") * Opt("long") * Flg("flg") * Opt("longer-opt", "l") * Pos("pos1") *
                          Flg("glf", "g") * Flg("f") * Opt("o");
    auto const program = Program().on_error(rethrow) + args + cmd;

    WHEN("an empty argv is parsed") {
      std::array<char const *, 0> argv;

      THEN("we should throw an error because of missing required arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("an argv with only 1 element is parsed") {
      auto argv = std::array{"./program"};

      THEN("we should throw an error because of missing required arguments") {
        REQUIRE_THROWS_AS(program(std::span(argv)), UserError);
      }
    }

    AND_WHEN("all arguments are parsed, all long names, positionals given last") {
      auto argv =
          std::array{"./program", "--long", "long-val", "--flg", "--longer-opt", "longer-opt-val", "--glf", "-f", "-o",
                     "o-val",     "a",      "b",        "cmd",   "--long",       "long-val",       "-f",    "aa", "bb"};

      auto const map = program(std::span(argv));

      THEN("exec_path should be set") { REQUIRE(map.exec_path == std::string_view(argv[0])); }

      AND_THEN("args should have all the arguments of the main program") {
        REQUIRE(map.args.size() == args.size());
        REQUIRE(map.as<std::string_view>("long") == std::string_view(argv[2]));
        REQUIRE(map.as<bool>("flg") == true);
        REQUIRE(map.as<std::string_view>("longer-opt") == std::string_view(argv[5]));
        REQUIRE(map.as<bool>("glf") == true);
        REQUIRE(map.as<bool>("f") == true);
        REQUIRE(map.as<std::string_view>("o") == std::string_view(argv[9]));
        REQUIRE(map.as<std::string_view>("pos2") == std::string_view(argv[10]));
        REQUIRE(map.as<std::string_view>("pos1") == std::string_view(argv[11]));
      }

      AND_THEN("cmd_name should be set") { REQUIRE(map.cmd_name == cmd.metadata.name); }
      AND_THEN("cmd_args should have all the arguments of the command") {
        REQUIRE(map.cmd_args != nullptr);

        auto const &cmd_args = *map.cmd_args;
        REQUIRE(cmd_args.as<std::string_view>("long") == std::string_view(argv[14]));
        REQUIRE(cmd_args.as<bool>("f") == true);
        REQUIRE(cmd_args.as<std::string_view>("pos1") == std::string_view(argv[16]));
        REQUIRE(cmd_args.as<std::string_view>("cmd-pos") == std::string_view(argv[17]));
      }
    }
  }
}
