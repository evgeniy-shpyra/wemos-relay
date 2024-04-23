#define ARDUINOJSON_STRING_LENGTH_SIZE 4
#include <ArduinoJson.h>

#include <catch.hpp>
#include <string>

TEST_CASE("ARDUINOJSON_STRING_LENGTH_SIZE == 4") {
  JsonDocument doc;

  SECTION("set() returns true if string has 65536 characters") {
    auto result = doc.set(std::string(65536, '?'));

    REQUIRE(result == true);
    REQUIRE(doc.overflowed() == false);
  }

  SECTION("deserializeJson() returns Ok if string has 65536 characters") {
    auto input = "\"" + std::string(65536, '?') + "\"";

    auto err = deserializeJson(doc, input);

    REQUIRE(err == DeserializationError::Ok);
  }

  SECTION("deserializeMsgPack() returns Ok of string has 65536 characters") {
    auto input = "\xda\xff\xff" + std::string(65536, '?');

    auto err = deserializeMsgPack(doc, input);

    REQUIRE(err == DeserializationError::Ok);
  }
}
