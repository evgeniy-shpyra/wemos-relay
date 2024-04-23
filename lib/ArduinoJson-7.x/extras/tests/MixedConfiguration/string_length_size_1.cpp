#define ARDUINOJSON_STRING_LENGTH_SIZE 1
#include <ArduinoJson.h>

#include <catch.hpp>
#include <string>

TEST_CASE("ARDUINOJSON_STRING_LENGTH_SIZE == 1") {
  JsonDocument doc;

  SECTION("set() returns true if string has 255 characters") {
    auto result = doc.set(std::string(255, '?'));

    REQUIRE(result == true);
    REQUIRE(doc.overflowed() == false);
  }

  SECTION("set() returns false if string has 256 characters") {
    auto result = doc.set(std::string(256, '?'));

    REQUIRE(result == false);
    REQUIRE(doc.overflowed() == true);
  }

  SECTION("deserializeJson() returns Ok if string has 255 characters") {
    auto input = "\"" + std::string(255, '?') + "\"";

    auto err = deserializeJson(doc, input);

    REQUIRE(err == DeserializationError::Ok);
  }

  SECTION("deserializeJson() returns NoMemory if string has 256 characters") {
    auto input = "\"" + std::string(256, '?') + "\"";

    auto err = deserializeJson(doc, input);

    REQUIRE(err == DeserializationError::NoMemory);
  }

  SECTION("deserializeMsgPack() returns Ok of string has 255 characters") {
    auto input = "\xd9\xff" + std::string(255, '?');

    auto err = deserializeMsgPack(doc, input);

    REQUIRE(err == DeserializationError::Ok);
  }

  SECTION(
      "deserializeMsgPack() returns NoMemory of string has 256 characters") {
    auto input = std::string("\xda\x01\x00", 3) + std::string(256, '?');

    auto err = deserializeMsgPack(doc, input);

    REQUIRE(err == DeserializationError::NoMemory);
  }
}
