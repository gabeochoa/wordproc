#include "../src/editor/spellcheck.h"

#include "catch2/catch.hpp"

// GrammarChecker (Tools > Grammar... dialog lists these, read-only).
// The E2E pins the repeated-word case: "The the cat sat." yields
// exactly one REPEATED_WORD issue.

TEST_CASE("GrammarChecker - repeated word", "[grammar]") {
    GrammarChecker checker;

    SECTION("detects a repeated word (pinned: Grammar E2E relies on it)") {
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "REPEATED_WORD");
        REQUIRE(errors[0].text == "The the");
        REQUIRE(errors[0].message == "Repeated word");
        REQUIRE(errors[0].suggestion == "The");
    }
    SECTION("clean text has no repeated-word error") {
        auto errors = checker.checkText("The cat sat.");
        REQUIRE(errors.empty());
    }
    SECTION("disabling the rule silences it") {
        checker.disableRule("REPEATED_WORD");
        REQUIRE(checker.isRuleEnabled("REPEATED_WORD") == false);
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.empty());
        checker.enableRule("REPEATED_WORD");
        REQUIRE(checker.isRuleEnabled("REPEATED_WORD") == true);
    }
}

TEST_CASE("GrammarChecker - capitalization, double spaces, common errors",
          "[grammar]") {
    GrammarChecker checker;

    SECTION("sentence must start with a capital") {
        auto errors = checker.checkText("hello. world");
        REQUIRE(errors.size() == 2);
        REQUIRE(errors[0].ruleId == "SENTENCE_CAPITALIZATION");
        REQUIRE(errors[0].text == "hello");
        REQUIRE(errors[0].suggestion == "Hello");
        REQUIRE(errors[1].text == "world");
    }
    SECTION("double space") {
        // Capitalized words: lowercase "a" would also trip
        // SENTENCE_CAPITALIZATION, so the fixture uses capitals.
        auto errors = checker.checkText("A  B");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "DOUBLE_SPACE");
        REQUIRE(errors[0].length == 2);
        REQUIRE(errors[0].suggestion == " ");
    }
    SECTION("common error: could of -> could have") {
        auto errors = checker.checkText("I could of gone.");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "COMMON_ERRORS");
        REQUIRE(errors[0].text == "could of");
        REQUIRE(errors[0].suggestion == "could have");
    }
    SECTION("clean sentence is clean") {
        auto errors = checker.checkText("The cat sat.");
        REQUIRE(errors.empty());
    }
    SECTION("available rules list the four engines") {
        auto rules = checker.availableRules();
        REQUIRE(rules.size() == 4);
    }
}
