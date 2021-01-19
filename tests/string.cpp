#include <string_view>

#include <catch2/catch.hpp>

#include "string.hpp"

// until we don't have fuzz testing...

SCENARIO("is_valid_name", "[string]") {
  using namespace opzioni;

  GIVEN("valid names") {
    REQUIRE(is_valid_name("a") == true);
    REQUIRE(is_valid_name("A") == true);

    REQUIRE(is_valid_name("aa") == true);
    REQUIRE(is_valid_name("aA") == true);
    REQUIRE(is_valid_name("Aa") == true);
    REQUIRE(is_valid_name("AA") == true);

    REQUIRE(is_valid_name("a0") == true);
    REQUIRE(is_valid_name("A0") == true);
    REQUIRE(is_valid_name("a0a") == true);
    REQUIRE(is_valid_name("A0A") == true);

    REQUIRE(is_valid_name("a-a") == true);
    REQUIRE(is_valid_name("A-a") == true);
    REQUIRE(is_valid_name("a-A") == true);
    REQUIRE(is_valid_name("A-A") == true);

    REQUIRE(is_valid_name("a_a") == true);
    REQUIRE(is_valid_name("A_a") == true);
    REQUIRE(is_valid_name("a_A") == true);
    REQUIRE(is_valid_name("A_A") == true);

    REQUIRE(is_valid_name("a-0") == true);
    REQUIRE(is_valid_name("A-0") == true);
    REQUIRE(is_valid_name("a_0") == true);
    REQUIRE(is_valid_name("A_0") == true);
  }

  GIVEN("an empty name") { REQUIRE(is_valid_name("") == false); }

  GIVEN("invalid names starting with numbers") {
    REQUIRE(is_valid_name("0") == false);
    REQUIRE(is_valid_name("1") == false);
    REQUIRE(is_valid_name("2") == false);
    REQUIRE(is_valid_name("3") == false);
    REQUIRE(is_valid_name("4") == false);
    REQUIRE(is_valid_name("5") == false);
    REQUIRE(is_valid_name("6") == false);
    REQUIRE(is_valid_name("7") == false);
    REQUIRE(is_valid_name("8") == false);
    REQUIRE(is_valid_name("9") == false);
    REQUIRE(is_valid_name("00") == false);
    REQUIRE(is_valid_name("0a") == false);
  }

  GIVEN("invalid names with -") {
    REQUIRE(is_valid_name("-") == false);
    REQUIRE(is_valid_name("--") == false);

    REQUIRE(is_valid_name("a-") == false);
    REQUIRE(is_valid_name("A-") == false);
    REQUIRE(is_valid_name("0-") == false);

    REQUIRE(is_valid_name("-a") == false);
    REQUIRE(is_valid_name("-A") == false);
    REQUIRE(is_valid_name("-0") == false);

    REQUIRE(is_valid_name("-a-") == false);
    REQUIRE(is_valid_name("-A-") == false);
    REQUIRE(is_valid_name("-0-") == false);

    REQUIRE(is_valid_name("0-a") == false);
    REQUIRE(is_valid_name("0-A") == false);
    REQUIRE(is_valid_name("0-0") == false);
  }

  GIVEN("invalid names with _") {
    REQUIRE(is_valid_name("_") == false);
    REQUIRE(is_valid_name("__") == false);

    REQUIRE(is_valid_name("a_") == false);
    REQUIRE(is_valid_name("A_") == false);
    REQUIRE(is_valid_name("0_") == false);

    REQUIRE(is_valid_name("_a") == false);
    REQUIRE(is_valid_name("_A") == false);
    REQUIRE(is_valid_name("_0") == false);

    REQUIRE(is_valid_name("0_a") == false);
    REQUIRE(is_valid_name("0_A") == false);
    REQUIRE(is_valid_name("0_0") == false);
  }

  GIVEN("invalid names with - and _") {
    REQUIRE(is_valid_name("-_") == false);
    REQUIRE(is_valid_name("_-") == false);

    REQUIRE(is_valid_name("-_-") == false);
    REQUIRE(is_valid_name("_-_") == false);
  }

  GIVEN("names containing blanks") {
    REQUIRE(is_valid_name(" ") == false);
    REQUIRE(is_valid_name("  ") == false);
    REQUIRE(is_valid_name("- ") == false);
    REQUIRE(is_valid_name(" -") == false);
    REQUIRE(is_valid_name("_ ") == false);
    REQUIRE(is_valid_name(" _") == false);
    REQUIRE(is_valid_name("a ") == false);
    REQUIRE(is_valid_name(" a") == false);
    REQUIRE(is_valid_name("0 ") == false);
    REQUIRE(is_valid_name(" 0") == false);

    REQUIRE(is_valid_name("a b") == false);

    REQUIRE(is_valid_name("\n") == false);
    REQUIRE(is_valid_name("a\n") == false);
    REQUIRE(is_valid_name("\nb") == false);
    REQUIRE(is_valid_name("a\nb") == false);

    REQUIRE(is_valid_name("\t") == false);
    REQUIRE(is_valid_name("a\t") == false);
    REQUIRE(is_valid_name("\tb") == false);
    REQUIRE(is_valid_name("a\tb") == false);

    REQUIRE(is_valid_name("\r\n") == false);
    REQUIRE(is_valid_name("a\r\n") == false);
    REQUIRE(is_valid_name("\r\nb") == false);
    REQUIRE(is_valid_name("a\r\nb") == false);
  }

  GIVEN("names containing other symbols") {
    REQUIRE(is_valid_name("`") == false);
    REQUIRE(is_valid_name("~") == false);
    REQUIRE(is_valid_name("!") == false);
    REQUIRE(is_valid_name("@") == false);
    REQUIRE(is_valid_name("#") == false);
    REQUIRE(is_valid_name("$") == false);
    REQUIRE(is_valid_name("%") == false);
    REQUIRE(is_valid_name("^") == false);
    REQUIRE(is_valid_name("&") == false);
    REQUIRE(is_valid_name("*") == false);
    REQUIRE(is_valid_name("(") == false);
    REQUIRE(is_valid_name(")") == false);
    REQUIRE(is_valid_name("=") == false);
    REQUIRE(is_valid_name("+") == false);
    REQUIRE(is_valid_name("[") == false);
    REQUIRE(is_valid_name("]") == false);
    REQUIRE(is_valid_name("{") == false);
    REQUIRE(is_valid_name("}") == false);
    REQUIRE(is_valid_name("|") == false);
    REQUIRE(is_valid_name(";") == false);
    REQUIRE(is_valid_name(":") == false);
    REQUIRE(is_valid_name("'") == false);
    REQUIRE(is_valid_name(",") == false);
    REQUIRE(is_valid_name(".") == false);
    REQUIRE(is_valid_name("<") == false);
    REQUIRE(is_valid_name(">") == false);
    REQUIRE(is_valid_name("/") == false);
    REQUIRE(is_valid_name("?") == false);
    REQUIRE(is_valid_name("\"") == false);
    REQUIRE(is_valid_name("\\") == false);

    REQUIRE(is_valid_name("a`") == false);
    REQUIRE(is_valid_name("a~") == false);
    REQUIRE(is_valid_name("a!") == false);
    REQUIRE(is_valid_name("a@") == false);
    REQUIRE(is_valid_name("a#") == false);
    REQUIRE(is_valid_name("a$") == false);
    REQUIRE(is_valid_name("a%") == false);
    REQUIRE(is_valid_name("a^") == false);
    REQUIRE(is_valid_name("a&") == false);
    REQUIRE(is_valid_name("a*") == false);
    REQUIRE(is_valid_name("a(") == false);
    REQUIRE(is_valid_name("a)") == false);
    REQUIRE(is_valid_name("a=") == false);
    REQUIRE(is_valid_name("a+") == false);
    REQUIRE(is_valid_name("a[") == false);
    REQUIRE(is_valid_name("a]") == false);
    REQUIRE(is_valid_name("a{") == false);
    REQUIRE(is_valid_name("a}") == false);
    REQUIRE(is_valid_name("a|") == false);
    REQUIRE(is_valid_name("a;") == false);
    REQUIRE(is_valid_name("a:") == false);
    REQUIRE(is_valid_name("a'") == false);
    REQUIRE(is_valid_name("a,") == false);
    REQUIRE(is_valid_name("a.") == false);
    REQUIRE(is_valid_name("a<") == false);
    REQUIRE(is_valid_name("a>") == false);
    REQUIRE(is_valid_name("a/") == false);
    REQUIRE(is_valid_name("a?") == false);
    REQUIRE(is_valid_name("a\"") == false);
    REQUIRE(is_valid_name("a\\") == false);

    REQUIRE(is_valid_name("a`a") == false);
    REQUIRE(is_valid_name("a~a") == false);
    REQUIRE(is_valid_name("a!a") == false);
    REQUIRE(is_valid_name("a@a") == false);
    REQUIRE(is_valid_name("a#a") == false);
    REQUIRE(is_valid_name("a$a") == false);
    REQUIRE(is_valid_name("a%a") == false);
    REQUIRE(is_valid_name("a^a") == false);
    REQUIRE(is_valid_name("a&a") == false);
    REQUIRE(is_valid_name("a*a") == false);
    REQUIRE(is_valid_name("a(a") == false);
    REQUIRE(is_valid_name("a)a") == false);
    REQUIRE(is_valid_name("a=a") == false);
    REQUIRE(is_valid_name("a+a") == false);
    REQUIRE(is_valid_name("a[a") == false);
    REQUIRE(is_valid_name("a]a") == false);
    REQUIRE(is_valid_name("a{a") == false);
    REQUIRE(is_valid_name("a}a") == false);
    REQUIRE(is_valid_name("a|a") == false);
    REQUIRE(is_valid_name("a;a") == false);
    REQUIRE(is_valid_name("a:a") == false);
    REQUIRE(is_valid_name("a'a") == false);
    REQUIRE(is_valid_name("a,a") == false);
    REQUIRE(is_valid_name("a.a") == false);
    REQUIRE(is_valid_name("a<a") == false);
    REQUIRE(is_valid_name("a>a") == false);
    REQUIRE(is_valid_name("a/a") == false);
    REQUIRE(is_valid_name("a?a") == false);
    REQUIRE(is_valid_name("a\"a") == false);
    REQUIRE(is_valid_name("a\\a") == false);
  }
}
