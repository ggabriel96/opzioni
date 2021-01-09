#include <variant>

#include <catch2/catch.hpp>

#include "opzioni.hpp"

SCENARIO("default values", "[Arg][defaults]") {
  using namespace opzioni;

  WHEN("Flg is called with one argument") {
    constexpr auto arg = Flg("flg");

    THEN("type should be FLG") { REQUIRE(arg.type == ArgType::FLG); }
    THEN("name should be equal to argument") { REQUIRE(arg.name == "flg"); }
    THEN("abbrev should be empty") { REQUIRE(arg.abbrev.empty()); }
    THEN("description should be empty") { REQUIRE(arg.description.empty()); }
    THEN("is_required should be false") { REQUIRE(!arg.is_required); }
    THEN("default_value should be false") { REQUIRE(std::get<bool>(arg.default_value) == false); }
    THEN("set_value should be true") { REQUIRE(std::get<bool>(arg.set_value) == true); }
    THEN("action_fn should be assign<bool>") { REQUIRE(arg.action_fn == actions::assign<bool>); }
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
    THEN("set_value should be true") { REQUIRE(std::get<bool>(arg.set_value) == true); }
    THEN("action_fn should be assign<bool>") { REQUIRE(arg.action_fn == actions::assign<bool>); }
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
    THEN("set_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.set_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == actions::assign<std::string_view>); }
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
    THEN("set_value should be empty") { REQUIRE(std::holds_alternative<std::monostate>(arg.set_value)); }
    THEN("action_fn should be assign<string_view>") { REQUIRE(arg.action_fn == actions::assign<std::string_view>); }
    THEN("gather_amount should be 1") { REQUIRE(arg.gather_amount == 1); }
    THEN("default_setter should be nullptr") { REQUIRE(arg.default_setter == nullptr); }
  }
}
