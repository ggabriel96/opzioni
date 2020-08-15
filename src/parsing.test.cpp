#include "parsing.hpp"

#include <array>
#include <string>

#include <catch2/catch.hpp>

SCENARIO( "positional arguments" ) {
  using namespace std::string_literals;
  opzioni::Program program;

  GIVEN( "only 1 argument" ) {
    program.pos({.name = "name"});

    WHEN( "the expected argument is given in CLI" ) {
      std::array argv{"./test", "someone"};
      opzioni::parsing::ArgumentParser parser(program, argv);

      THEN( "it is parsed as positional" ) {
        auto const result = parser();
        
        REQUIRE( result.cmd_name == "./test" );
        REQUIRE( result.subcmd == nullptr );
        REQUIRE( result.options.empty() );
        REQUIRE( result.flags.empty() );
        REQUIRE( result.positional.size() == 1 );
        REQUIRE( result.positional[0] == "someone"s );
      }
    }
  }
}
