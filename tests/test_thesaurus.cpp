#include "../src/editor/thesaurus.h"

#include "catch2/catch.hpp"

TEST_CASE("Thesaurus parse - entries, comments, malformed lines",
          "[thesaurus]") {
    const std::string content =
        "# comment line\n"
        "\n"
        "happy: joyful, cheerful, glad\n"
        "bad line without colon\n"
        "empty:\n"
        "  Big : LARGE , Huge  \n"
        "dup: one\n"
        "dup: two, three\n";
    auto entries = thesaurus::parse(content);

    SECTION("Parses headword and synonyms") {
        REQUIRE(entries.count("happy") == 1);
        REQUIRE(entries.at("happy") ==
                std::vector<std::string>({"joyful", "cheerful", "glad"}));
    }
    SECTION("Comments, blanks, and colon-less lines are skipped") {
        REQUIRE(entries.count("# comment line") == 0);
        REQUIRE(entries.count("bad line without colon") == 0);
    }
    SECTION("An entry with no synonyms is skipped") {
        REQUIRE(entries.count("empty") == 0);
    }
    SECTION("Headwords and synonyms are trimmed and lowercased") {
        REQUIRE(entries.count("big") == 1);
        REQUIRE(entries.at("big") ==
                std::vector<std::string>({"large", "huge"}));
    }
    SECTION("A later entry for the same word replaces the earlier one") {
        REQUIRE(entries.at("dup") ==
                std::vector<std::string>({"two", "three"}));
    }
}

TEST_CASE("Thesaurus lookup - case-insensitive, missing word",
          "[thesaurus]") {
    auto entries = thesaurus::parse("happy: joyful, cheerful\n");

    SECTION("Lookup is case-insensitive and trims") {
        REQUIRE(thesaurus::lookup(entries, "HAPPY") ==
                std::vector<std::string>({"joyful", "cheerful"}));
        REQUIRE(thesaurus::lookup(entries, "  happy ") ==
                std::vector<std::string>({"joyful", "cheerful"}));
    }
    SECTION("A missing word returns an empty list") {
        REQUIRE(thesaurus::lookup(entries, "zxqv").empty());
        REQUIRE(thesaurus::lookup(entries, "").empty());
    }
}

TEST_CASE("Thesaurus shipped file - loads, ~120 real entries",
          "[thesaurus]") {
    auto entries = thesaurus::loadDefault();

    SECTION("The shipped file resolves and parses") {
        REQUIRE_FALSE(entries.empty());
        REQUIRE(entries.size() >= 100);
        REQUIRE(entries.size() <= 140);
    }
    SECTION("Brief-pinned words have real synonyms") {
        REQUIRE_FALSE(thesaurus::lookup(entries, "happy").empty());
        REQUIRE(thesaurus::lookup(entries, "happy").at(0) == "joyful");
        REQUIRE_FALSE(thesaurus::lookup(entries, "begin").empty());
        REQUIRE_FALSE(thesaurus::lookup(entries, "different").empty());
        REQUIRE_FALSE(thesaurus::lookup(entries, "important").empty());
    }
    SECTION("Every synonym is lowercase alphabetic (no gibberish)") {
        for (const auto& [word, synonyms] : entries) {
            REQUIRE_FALSE(synonyms.empty());
            for (const auto& syn : synonyms) {
                REQUIRE_FALSE(syn.empty());
                for (char ch : syn) {
                    REQUIRE(((ch >= 'a' && ch <= 'z') || ch == ' ' ||
                             ch == '-'));
                }
                REQUIRE(syn != word);
            }
        }
    }
    SECTION("A word with no entry returns empty (dialog shows "
            "\"(no thesaurus entry)\")") {
        REQUIRE(thesaurus::lookup(entries, "zxqv").empty());
        REQUIRE(thesaurus::lookup(entries, "glad").empty());
    }
}
