#include <ArduinoJson.h>

#include <catch.hpp>

#include "Literals.hpp"

TEST_CASE("Issue #1120") {
  JsonDocument doc;
  constexpr char str[] =
      "{\"contents\":[{\"module\":\"Packet\"},{\"module\":\"Analog\"}]}";
  deserializeJson(doc, str);

  SECTION("MemberProxy<std::string>::isNull()") {
    SECTION("returns false") {
      CHECK(doc["contents"_s].isNull() == false);
    }

    SECTION("returns true") {
      CHECK(doc["zontents"_s].isNull() == true);
    }
  }

  SECTION("ElementProxy<MemberProxy<const char*> >::isNull()") {
    SECTION("returns false") {  // Issue #1120
      CHECK(doc["contents"][1].isNull() == false);
    }

    SECTION("returns true") {
      CHECK(doc["contents"][2].isNull() == true);
    }
  }

  SECTION("MemberProxy<ElementProxy<MemberProxy>, const char*>::isNull()") {
    SECTION("returns false") {
      CHECK(doc["contents"][1]["module"].isNull() == false);
    }

    SECTION("returns true") {
      CHECK(doc["contents"][1]["zodule"].isNull() == true);
    }
  }

  SECTION("MemberProxy<ElementProxy<MemberProxy>, std::string>::isNull()") {
    SECTION("returns false") {
      CHECK(doc["contents"][1]["module"_s].isNull() == false);
    }

    SECTION("returns true") {
      CHECK(doc["contents"][1]["zodule"_s].isNull() == true);
    }
  }
}
