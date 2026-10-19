#include <set>

#include "../src/editor/symbol.h"
#include "catch2/catch.hpp"

TEST_CASE("Symbol grid", "[symbol]") {
    const auto& entries = symbol::entries();

    SECTION("all required characters are present") {
        auto contains = [&](const char* ch) {
            for (const auto& e : entries) {
                if (std::string(e.character) == ch) return true;
            }
            return false;
        };
        REQUIRE(contains("\xC2\xA9"));  // ©
        REQUIRE(contains("\xC2\xAE"));  // ®
        REQUIRE(contains("\xE2\x84\xA2"));  // ™
        REQUIRE(contains("\xC2\xA7"));  // §
        REQUIRE(contains("\xC2\xB6"));  // ¶
        REQUIRE(contains("\xE2\x80\xA2"));  // •
        REQUIRE(contains("\xE2\x80\x93"));  // –
        REQUIRE(contains("\xE2\x80\x94"));  // —
        REQUIRE(contains("\xE2\x80\x98"));  // '
        REQUIRE(contains("\xE2\x80\x99"));  // '
        REQUIRE(contains("\xE2\x80\x9C"));  // "
        REQUIRE(contains("\xE2\x80\x9D"));  // "
        REQUIRE(contains("\xC2\xAB"));  // «
        REQUIRE(contains("\xC2\xBB"));  // »
        REQUIRE(contains("\xE2\x82\xAC"));  // €
        REQUIRE(contains("\xC2\xA3"));  // £
        REQUIRE(contains("\xC2\xA5"));  // ¥
        REQUIRE(contains("\xC2\xB0"));  // °
        REQUIRE(contains("\xC2\xB1"));  // ±
        REQUIRE(contains("\xC2\xBD"));  // ½
        REQUIRE(contains("\xC3\xB7"));  // ÷
        REQUIRE(contains("\xC3\x97"));  // ×
        REQUIRE(contains("\xE2\x80\xA6"));  // …
        REQUIRE(contains("\xCE\xB1"));  // α
        REQUIRE(contains("\xCF\x89"));  // ω
    }

    SECTION("entries are unique and well-formed") {
        REQUIRE(entries.size() == 47);  // 23 symbols + 24 Greek
        std::set<std::string> chars;
        std::set<std::string> slugs;
        for (const auto& e : entries) {
            REQUIRE(e.character != nullptr);
            REQUIRE(e.name != nullptr);
            REQUIRE(e.slug != nullptr);
            REQUIRE(std::string(e.character).size() >= 2);
            REQUIRE(std::string(e.name).size() > 0);
            REQUIRE(std::string(e.slug).size() > 0);
            REQUIRE(chars.insert(e.character).second);
            REQUIRE(slugs.insert(e.slug).second);
        }
    }

    SECTION("Greek alpha-omega are the last 24 entries, in order") {
        REQUIRE(entries.size() >= 24);
        auto tail = std::vector<symbol::Entry>(
            entries.end() - 24, entries.end());
        REQUIRE(std::string(tail.front().character) == "\xCE\xB1");
        REQUIRE(std::string(tail.back().character) == "\xCF\x89");
        REQUIRE(std::string(tail.front().name) == "Alpha");
        REQUIRE(std::string(tail.back().name) == "Omega");
    }
}
