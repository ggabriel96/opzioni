#include <variant>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("default values", "[Arg][defaults]") {
  using namespace opzioni;

  WHEN("Arg is default constructed") {
    constexpr auto arg = Arg();

    THEN("type should be POS") { REQUIRE(arg.type == ArgType::POS); }
    THEN("name should be empty") { REQUIRE(arg.name.empty()); }
    THEN("abbrev should be empty") { REQUIRE(arg.abbrev.empty()); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be true") { REQUIRE(arg.is_required); }
    THEN("default_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.default_value)); }
    THEN("implicit_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.implicit_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == act::fn::assign<std::string_view>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Flg is called with one argument") {
    constexpr auto arg = Flg("flg");

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to argument") { REQUIRE(arg.name == "flg"); }
    THEN("abbrev should be empty") { REQUIRE(arg.abbrev.empty()); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be assign<bool>") { REQUIRE(arg.action_fn == act::fn::assign<bool>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Flg is called with two arguments") {
    constexpr auto arg = Flg("flg", "f");

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to first argument") { REQUIRE(arg.name == "flg"); }
    THEN("abbrev should be equal to second argument") { REQUIRE(arg.abbrev == "f"); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be assign<bool>") { REQUIRE(arg.action_fn == act::fn::assign<bool>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Opt is called with one argument") {
    constexpr auto arg = Opt("opt");

    THEN("type should be OPT") { REQUIRE(arg.type == ArgType::OPT); }
    THEN("name should be equal to argument") { REQUIRE(arg.name == "opt"); }
    THEN("abbrev should be empty") { REQUIRE(arg.abbrev.empty()); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be empty string") { REQUIRE(std::get<std::string_view>(arg.default_value) == ""); }
    THEN("implicit_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.implicit_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == act::fn::assign<std::string_view>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Opt is called with two arguments") {
    constexpr auto arg = Opt("opt", "o");

    THEN("type should be OPT") { REQUIRE(arg.type == ArgType::OPT); }
    THEN("name should be equal to first argument") { REQUIRE(arg.name == "opt"); }
    THEN("abbrev should be equal to second argument") { REQUIRE(arg.abbrev == "o"); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be empty string") { REQUIRE(std::get<std::string_view>(arg.default_value) == ""); }
    THEN("implicit_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.implicit_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == act::fn::assign<std::string_view>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Pos is called with one argument") {
    constexpr auto arg = Pos("pos");

    THEN("type should be POS") { REQUIRE(arg.type == ArgType::POS); }
    THEN("name should be equal to argument") { REQUIRE(arg.name == "pos"); }
    THEN("abbrev should be empty") { REQUIRE(arg.abbrev.empty()); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be true") { REQUIRE(arg.is_required); }
    THEN("default_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.default_value)); }
    THEN("implicit_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.implicit_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == act::fn::assign<std::string_view>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Help is called with no argument") {
    constexpr auto arg = Help();

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to help") { REQUIRE(arg.name == "help"); }
    THEN("abbrev should be equal to h") { REQUIRE(arg.abbrev == "h"); }
    THEN("description should be the default") { REQUIRE(arg.description == "Display this information"); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be print_help") { REQUIRE(arg.action_fn == act::fn::print_help); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Help is called with one argument") {
    constexpr auto arg = Help("Display this help text and exit");

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to help") { REQUIRE(arg.name == "help"); }
    THEN("abbrev should be equal to h") { REQUIRE(arg.abbrev == "h"); }
    THEN("description should be equal to argument") { REQUIRE(arg.description == "Display this help text and exit"); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be print_help") { REQUIRE(arg.action_fn == act::fn::print_help); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Version is called with no argument") {
    constexpr auto arg = Version();

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to version") { REQUIRE(arg.name == "version"); }
    THEN("abbrev should be equal to V") { REQUIRE(arg.abbrev == "V"); }
    THEN("description should be the default") { REQUIRE(arg.description == "Display the software version"); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be print_version") { REQUIRE(arg.action_fn == act::fn::print_version); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }

  WHEN("Version is called with one argument") {
    constexpr auto arg = Version("Show the version of this program");

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to version") { REQUIRE(arg.name == "version"); }
    THEN("abbrev should be equal to V") { REQUIRE(arg.abbrev == "V"); }
    THEN("description should be equal to argument") { REQUIRE(arg.description == "Show the version of this program"); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("implicit_value should be true") { REQUIRE(std::get<bool>(arg.implicit_value) == true); }
    THEN("action_fn should be print_version") { REQUIRE(arg.action_fn == act::fn::print_version); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }
}

SCENARIO("has_abbrev", "[Arg]") {
  using namespace opzioni;

  GIVEN("an Arg with a default-constructed abbrev") {
    constexpr auto arg = Arg{.abbrev{}};
    THEN("has_abbrev should return false") { REQUIRE(!arg.has_abbrev()); }
  }

  GIVEN("an Arg with an empty-string abbrev") {
    constexpr auto arg = Arg{.abbrev = ""};
    THEN("has_abbrev should return false") { REQUIRE(!arg.has_abbrev()); }
  }

  GIVEN("an Arg with an one-letter abbrev") {
    constexpr auto arg = Arg{.abbrev = "a"};
    THEN("has_abbrev should return true") { REQUIRE(arg.has_abbrev()); }
  }

  GIVEN("an Arg with an single blank abbrev") {
    constexpr auto arg = Arg{.abbrev = " "};
    THEN("has_abbrev should return true") { REQUIRE(arg.has_abbrev()); }
  }

  GIVEN("an Arg with an many-letter abbrev") {
    constexpr auto arg = Arg{.abbrev = "abc"};
    THEN("has_abbrev should return true") { REQUIRE(arg.has_abbrev()); }
  }

  GIVEN("an Arg with an many blanks abbrev") {
    constexpr auto arg = Arg{.abbrev = "   "};
    THEN("has_abbrev should return true") { REQUIRE(arg.has_abbrev()); }
  }
}

SCENARIO("has_default", "[Arg]") {
  using namespace opzioni;

  GIVEN("an Arg with a default-constructed default_value") {
    constexpr auto arg = Arg{.default_value{}};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default-constructed default_setter") {
    constexpr auto arg = Arg{.default_setter{}};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default-constructed default_value and default_setter") {
    constexpr auto arg = Arg{.default_value{}, .default_setter{}};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default_value explicitly set to monostate") {
    constexpr auto arg = Arg{.default_value = std::monostate{}, .default_setter{}};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default_setter explicitly set to nullptr") {
    constexpr auto arg = Arg{.default_value{}, .default_setter = nullptr};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default_setter explicitly set to NULL") {
    constexpr auto arg = Arg{.default_value{}, .default_setter = NULL};
    THEN("has_default should return false") { REQUIRE(!arg.has_default()); }
  }

  GIVEN("an Arg with a default_value set to zero") {
    constexpr auto arg = Arg{.default_value = 0, .default_setter{}};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }

  GIVEN("an Arg with a default_value set to empty string") {
    constexpr auto arg = Arg{.default_value = "", .default_setter{}};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }

  GIVEN("an Arg with a default_value set to false") {
    constexpr auto arg = Arg{.default_value = false, .default_setter{}};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }

  GIVEN("an Arg with a default_setter set to an empty lambda") {
    constexpr auto arg = Arg{.default_value{}, .default_setter = +[](ArgValue &) {}};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }

  GIVEN("an Arg with a default_setter set to some non-empty lambda") {
    constexpr auto arg = Arg{.default_value{}, .default_setter = +[](ArgValue &argval) { argval.value = 0; }};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }

  GIVEN("an Arg with both a default_value and a default_setter set to an empty lambda") {
    // this would actually fail our validations in `validate_arg`
    constexpr auto arg = Arg{.default_value = 0, .default_setter = +[](ArgValue &) {}};
    THEN("has_default should return true") { REQUIRE(arg.has_default()); }
  }
}

SCENARIO("has_implicit", "[Arg]") {
  using namespace opzioni;

  GIVEN("an Arg with a default-constructed implicit_value") {
    constexpr auto arg = Arg{.implicit_value{}};
    THEN("has_implicit should return false") { REQUIRE(!arg.has_implicit()); }
  }

  GIVEN("an Arg with a implicit_value explicitly set to monostate") {
    constexpr auto arg = Arg{.implicit_value = std::monostate{}};
    THEN("has_implicit should return false") { REQUIRE(!arg.has_implicit()); }
  }

  GIVEN("an Arg with a implicit_value set to zero") {
    constexpr auto arg = Arg{.implicit_value = 0};
    THEN("has_implicit should return true") { REQUIRE(arg.has_implicit()); }
  }

  GIVEN("an Arg with a implicit_value set to empty string") {
    constexpr auto arg = Arg{.implicit_value = ""};
    THEN("has_implicit should return true") { REQUIRE(arg.has_implicit()); }
  }

  GIVEN("an Arg with a implicit_value set to false") {
    constexpr auto arg = Arg{.implicit_value = false};
    THEN("has_implicit should return true") { REQUIRE(arg.has_implicit()); }
  }
}

SCENARIO("is_positional", "[Arg]") {
  using namespace opzioni;

  GIVEN("an Arg with default-constructed type") {
    constexpr auto arg = Arg{.type{}};
    THEN("is_positional should return true") { REQUIRE(arg.is_positional()); }
  }

  GIVEN("an Arg with type set to POS") {
    constexpr auto arg = Arg{.type = ArgType::POS};
    THEN("is_positional should return true") { REQUIRE(arg.is_positional()); }
  }

  GIVEN("an Arg with type set to FLG") {
    constexpr auto arg = Arg{.type = ArgType::FLG};
    THEN("is_positional should return true") { REQUIRE(!arg.is_positional()); }
  }

  GIVEN("an Arg with type set to OPT") {
    constexpr auto arg = Arg{.type = ArgType::OPT};
    THEN("is_positional should return true") { REQUIRE(!arg.is_positional()); }
  }
}

SCENARIO("set_default_to", "[Arg]") {
  using namespace opzioni;

  GIVEN("an Arg without default_value and default_setter") {
    constexpr auto arg = Arg{.default_value = std::monostate{}, .default_setter = nullptr};

    WHEN("calling set_default_to with an empty ArgValue") {
      ArgValue argval{};
      REQUIRE(argval.value.index() == 0);

      arg.set_default_to(argval);

      THEN("the ArgValue should remain empty") {
        REQUIRE(argval.value.index() == 0);
        REQUIRE(std::holds_alternative<std::monostate>(argval.value));
      }
    }
  }

  GIVEN("an Arg with default_value and without default_setter") {
    constexpr auto arg = Arg{.default_value = 1, .default_setter = nullptr};

    WHEN("calling set_default_to with an empty ArgValue") {
      ArgValue argval{};
      REQUIRE(argval.value.index() == 0);

      arg.set_default_to(argval);

      THEN("the ArgValue should be set to the value from default_value") {
        REQUIRE(argval.value.index() != 0);
        REQUIRE(std::holds_alternative<int>(argval.value));
        REQUIRE(std::get<int>(argval.value) == 1);
      }
    }
  }

  GIVEN("an Arg without default_value and with default_setter") {
    constexpr auto arg = Arg{.default_value = std::monostate{},
                             .default_setter = [](ArgValue &argval) { argval.value = "from setter"; }};

    WHEN("calling set_default_to with an empty ArgValue") {
      ArgValue argval{};
      REQUIRE(argval.value.index() == 0);

      arg.set_default_to(argval);

      THEN("the ArgValue should be set to the value from default_setter") {
        REQUIRE(argval.value.index() != 0);
        REQUIRE(std::holds_alternative<std::string_view>(argval.value));
        REQUIRE(std::get<std::string_view>(argval.value) == "from setter");
      }
    }
  }

  GIVEN("an Arg with default_value and with default_setter") {
    // this shouldn't happen, but testing anyway
    constexpr auto arg =
        Arg{.default_value = "from default", .default_setter = [](ArgValue &argval) { argval.value = "from setter"; }};

    WHEN("calling set_default_to with an empty ArgValue") {
      ArgValue argval{};
      REQUIRE(argval.value.index() == 0);

      arg.set_default_to(argval);

      THEN("the ArgValue should be set to the value from default_setter") {
        REQUIRE(argval.value.index() != 0);
        REQUIRE(std::holds_alternative<std::string_view>(argval.value));
        REQUIRE(std::get<std::string_view>(argval.value) == "from setter");
      }
    }
  }
}
