#include <algorithm>
#include <array>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("setting general information", "[Program][defaults][info][setters]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("a default-initialized Program") {
    Program program;

    THEN("all member variables should have their defaults") {
      REQUIRE(program.name.empty());
      REQUIRE(program.version.empty());
      REQUIRE(program.title.empty());
      REQUIRE(program.introduction.empty());
      REQUIRE(program.description.empty());
      REQUIRE(program.msg_width == 100);
      REQUIRE(program.error_handler == print_error);
      REQUIRE(program.positionals_amount == 0);
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
    }

    WHEN("intro is called") {
      program.intro("intro");

      THEN("only the introduction should be changed") {
        REQUIRE(program.introduction == "intro");
        REQUIRE(program.name.empty());
        REQUIRE(program.version.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.description.empty());
        REQUIRE(program.msg_width == 100);
        REQUIRE(program.error_handler == print_error);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("details is called") {
      program.details("details");

      THEN("only the description should be changed") {
        REQUIRE(program.description == "details");
        REQUIRE(program.name.empty());
        REQUIRE(program.version.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.introduction.empty());
        REQUIRE(program.msg_width == 100);
        REQUIRE(program.error_handler == print_error);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("v is called") {
      program.v("1.0");

      THEN("only the version should be changed") {
        REQUIRE(program.version == "1.0");
        REQUIRE(program.name.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.introduction.empty());
        REQUIRE(program.description.empty());
        REQUIRE(program.msg_width == 100);
        REQUIRE(program.error_handler == print_error);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("intro, details, and v are called") {
      program.intro("intro").details("details").v("1.0");

      THEN("all three should be changed") {
        REQUIRE(program.introduction == "intro");
        REQUIRE(program.description == "details");
        REQUIRE(program.version == "1.0");
        REQUIRE(program.name.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.msg_width == 100);
        REQUIRE(program.error_handler == print_error);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("max_width is called") {
      program.max_width(80);

      THEN("only the msg_width should be changed") {
        REQUIRE(program.msg_width == 80);
        REQUIRE(program.name.empty());
        REQUIRE(program.version.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.introduction.empty());
        REQUIRE(program.description.empty());
        REQUIRE(program.error_handler == print_error);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("on_error is called") {
      program.on_error(nullptr);

      THEN("only the error_handler should be changed") {
        REQUIRE(program.error_handler == nullptr);
        REQUIRE(program.name.empty());
        REQUIRE(program.version.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.introduction.empty());
        REQUIRE(program.description.empty());
        REQUIRE(program.msg_width == 100);
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }

    WHEN("intro, details, v, max_width, and on_error are called") {
      program.intro("intro").details("details").v("1.0").max_width(80).on_error(nullptr);

      THEN("all five should be changed") {
        REQUIRE(program.introduction == "intro");
        REQUIRE(program.description == "details");
        REQUIRE(program.version == "1.0");
        REQUIRE(program.msg_width == 80);
        REQUIRE(program.error_handler == nullptr);
        REQUIRE(program.name.empty());
        REQUIRE(program.title.empty());
        REQUIRE(program.positionals_amount == 0);
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.cmds().size() == 0);
      }
    }
  }

  GIVEN("a Program initialized with a name") {
    Program program("program");

    THEN("only the name should not have its default value") {
      REQUIRE(program.name == "program");
      REQUIRE(program.version.empty());
      REQUIRE(program.title.empty());
      REQUIRE(program.introduction.empty());
      REQUIRE(program.description.empty());
      REQUIRE(program.msg_width == 100);
      REQUIRE(program.error_handler == print_error);
      REQUIRE(program.positionals_amount == 0);
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
    }
  }

  GIVEN("a Program initialized with a name and title") {
    Program program("program", "title");

    THEN("only name and title should not have their default values") {
      REQUIRE(program.name == "program");
      REQUIRE(program.title == "title");
      REQUIRE(program.version.empty());
      REQUIRE(program.introduction.empty());
      REQUIRE(program.description.empty());
      REQUIRE(program.msg_width == 100);
      REQUIRE(program.error_handler == print_error);
      REQUIRE(program.positionals_amount == 0);
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
      REQUIRE(program.positionals_amount == 0);
    }

    WHEN("a positional is added") {
      program + std::array{Pos("pos")};

      REQUIRE(program.args().size() == 1);
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 1); }
      THEN("it should be added as first argument") { REQUIRE(program.args()[0].name == "pos"); }
    }

    WHEN("two positionals are added") {
      program + Pos("pos1") * Pos("pos2");

      REQUIRE(program.args().size() == 2);
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
      THEN("positionals_amount should be incremented") { REQUIRE(program.positionals_amount == 2); }
      THEN("they should be added as first arguments, but preserving the order of insertion") {
        REQUIRE(program.args()[0].name == "pos1");
        REQUIRE(program.args()[1].name == "pos2");
      }
    }

    WHEN("multiple positionals are added") {
      program + Pos("pos5") * Pos("pos2") * Pos("pos4") * Pos("pos1") * Pos("pos3");

      REQUIRE(program.args().size() == 5);
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
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
      THEN("cmds should not be changed") { REQUIRE(program.cmds().size() == 0); }
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

SCENARIO("adding commands", "[Program][cmds]") {
  using namespace opzioni;
  using namespace std::string_view_literals;

  GIVEN("program and cmd") {
    auto const cmd_name = "cmd"sv;
    Program program;
    Program cmd(cmd_name);

    THEN("they should initially have no arguments") {
      REQUIRE(program.args().size() == 0);
      REQUIRE(program.cmds().size() == 0);
      REQUIRE(program.positionals_amount == 0);

      REQUIRE(cmd.args().size() == 0);
      REQUIRE(cmd.cmds().size() == 0);
      REQUIRE(cmd.positionals_amount == 0);
    }

    WHEN("cmd is added as command of program") {
      program + Cmd(cmd);

      THEN("cmd should be added as only command of program") {
        REQUIRE(program.cmds().size() == 1);
        REQUIRE(program.cmds()[0].program == &cmd);
      }
      THEN("program's args should not be changed") {
        REQUIRE(program.args().size() == 0);
        REQUIRE(program.positionals_amount == 0);
      }
      THEN("cmd should not be changed") {
        REQUIRE(cmd.name == cmd_name);
        REQUIRE(cmd.args().size() == 0);
        REQUIRE(cmd.cmds().size() == 0);
        REQUIRE(cmd.positionals_amount == 0);
      }

      AND_GIVEN("another command, cmd2") {
        auto const cmd2_name = "cmd2";
        Program cmd2(cmd2_name);

        THEN("cmd2 should initially have no arguments") {
          REQUIRE(cmd2.args().size() == 0);
          REQUIRE(cmd2.cmds().size() == 0);
          REQUIRE(cmd2.positionals_amount == 0);
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
            REQUIRE(program.positionals_amount == 0);
          }
          THEN("cmd should not be changed") {
            REQUIRE(cmd.name == cmd_name);
            REQUIRE(cmd.args().size() == 0);
            REQUIRE(cmd.cmds().size() == 0);
            REQUIRE(cmd.positionals_amount == 0);
          }
          THEN("cmd2 should not be changed") {
            REQUIRE(cmd2.name == cmd2_name);
            REQUIRE(cmd2.args().size() == 0);
            REQUIRE(cmd2.cmds().size() == 0);
            REQUIRE(cmd2.positionals_amount == 0);
          }
        }
      }
    }
  }
}
