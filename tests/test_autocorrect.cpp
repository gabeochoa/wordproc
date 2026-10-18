#include "../src/editor/autocorrect.h"
#include "../src/editor/spellcheck.h"

#include "catch2/catch.hpp"

// ============================================================================
// AutoCorrect Tests
// ============================================================================

TEST_CASE("AutoCorrect - Replacement table", "[autocorrect]") {
    AutoCorrect ac;

    SECTION("Common misspellings are corrected") {
        REQUIRE(ac.correct("teh", false) == std::optional<std::string>("the"));
        REQUIRE(ac.correct("adress", false) == std::optional<std::string>("address"));
        REQUIRE(ac.correct("recieve", false) == std::optional<std::string>("receive"));
        REQUIRE(ac.correct("seperate", false) == std::optional<std::string>("separate"));
        REQUIRE(ac.correct("definately", false) ==
                std::optional<std::string>("definitely"));
        REQUIRE(ac.correct("occured", false) == std::optional<std::string>("occurred"));
        REQUIRE(ac.correct("untill", false) == std::optional<std::string>("until"));
        REQUIRE(ac.correct("wierd", false) == std::optional<std::string>("weird"));
        REQUIRE(ac.correct("thier", false) == std::optional<std::string>("their"));
        REQUIRE(ac.correct("alot", false) == std::optional<std::string>("a lot"));
    }

    SECTION("Table has ~30 entries") {
        REQUIRE(AutoCorrect::table().size() >= 20);
    }

    SECTION("Correct words are untouched") {
        REQUIRE_FALSE(ac.correct("the", false).has_value());
        REQUIRE_FALSE(ac.correct("hello", false).has_value());
        REQUIRE_FALSE(ac.correct("world", false).has_value());
        REQUIRE_FALSE(ac.correct("address", false).has_value());
    }

    SECTION("Unknown words are untouched") {
        REQUIRE_FALSE(ac.correct("zxqv", false).has_value());
    }

    SECTION("Corrections are themselves spelled correctly") {
        // Otherwise the fix would immediately earn a red squiggle.
        SpellChecker checker;
        for (const auto& [wrong, right] : AutoCorrect::table()) {
            (void)wrong;
            for (const auto& [offset, word] :
                 SpellChecker::extractWords(right)) {
                (void)offset;
                REQUIRE(checker.isCorrect(word));
            }
        }
    }
}

TEST_CASE("AutoCorrect::entries - Sorted Replace/With pairs",
          "[autocorrect]") {
    auto entries = AutoCorrect::entries();
    REQUIRE(entries.size() == AutoCorrect::table().size());
    REQUIRE_FALSE(entries.empty());
    for (std::size_t i = 1; i < entries.size(); ++i) {
        REQUIRE(entries[i - 1].first < entries[i].first);
    }
    bool foundTeh = false;
    for (const auto& [from, to] : entries) {
        if (from == "teh") {
            REQUIRE(to == "the");
            foundTeh = true;
        }
    }
    REQUIRE(foundTeh);
}

TEST_CASE("AutoCorrect - Case preservation", "[autocorrect]") {
    AutoCorrect ac;

    SECTION("Capitalized original -> capitalized correction") {
        REQUIRE(ac.correct("Teh", false) == std::optional<std::string>("The"));
    }

    SECTION("All-caps original -> all-caps correction") {
        REQUIRE(ac.correct("TEH", false) == std::optional<std::string>("THE"));
    }

    SECTION("Lowercase original -> lowercase correction") {
        REQUIRE(ac.correct("teh", false) == std::optional<std::string>("the"));
    }

    SECTION("Standalone 'i' becomes 'I'") {
        REQUIRE(ac.correct("i", false) == std::optional<std::string>("I"));
        REQUIRE_FALSE(ac.correct("I", false).has_value());
    }

    SECTION("Contractions gain their apostrophe") {
        REQUIRE(ac.correct("dont", false) == std::optional<std::string>("don't"));
        REQUIRE(ac.correct("Dont", false) == std::optional<std::string>("Don't"));
    }
}

TEST_CASE("AutoCorrect - Sentence capitalization", "[autocorrect]") {
    AutoCorrect ac;

    SECTION("Lowercase word at a sentence start is capitalized") {
        REQUIRE(ac.correct("hello", true) == std::optional<std::string>("Hello"));
    }

    SECTION("Same word mid-sentence is untouched") {
        REQUIRE_FALSE(ac.correct("hello", false).has_value());
    }

    SECTION("Already-capitalized word is untouched") {
        REQUIRE_FALSE(ac.correct("Hello", true).has_value());
    }

    SECTION("Table fix at a sentence start is also capitalized") {
        REQUIRE(ac.correct("teh", true) == std::optional<std::string>("The"));
    }
}

TEST_CASE("AutoCorrect - Two initial capitals", "[autocorrect]") {
    AutoCorrect ac;

    SECTION("TWo -> Two") {
        REQUIRE(ac.correct("TWo", false) == std::optional<std::string>("Two"));
        REQUIRE(ac.correct("THis", false) == std::optional<std::string>("This"));
        REQUIRE(ac.correct("ABc", false) == std::optional<std::string>("Abc"));
    }

    SECTION("Acronyms and short words are untouched") {
        REQUIRE_FALSE(ac.correct("USA", false).has_value());
        REQUIRE_FALSE(ac.correct("TW", false).has_value());
        REQUIRE_FALSE(ac.correct("Two", false).has_value());
        REQUIRE_FALSE(ac.correct("The", false).has_value());
    }
}

TEST_CASE("AutoCorrect - Toggles disable each behavior", "[autocorrect]") {
    SECTION("All off: nothing is corrected") {
        AutoCorrect ac;
        ac.settings.replaceTextAsYouType = false;
        ac.settings.capitalizeFirstLetterOfSentence = false;
        ac.settings.correctTwoInitialCapitals = false;
        REQUIRE_FALSE(ac.correct("teh", true).has_value());
        REQUIRE_FALSE(ac.correct("TWo", true).has_value());
        REQUIRE_FALSE(ac.correct("hello", true).has_value());
    }

    SECTION("Replace off, capitalize on: table skipped, cap still applies") {
        AutoCorrect ac;
        ac.settings.replaceTextAsYouType = false;
        REQUIRE_FALSE(ac.correct("teh", false).has_value());
        REQUIRE(ac.correct("teh", true) == std::optional<std::string>("Teh"));
    }

    SECTION("Two-initials off: TWo untouched, table still applies") {
        AutoCorrect ac;
        ac.settings.correctTwoInitialCapitals = false;
        REQUIRE_FALSE(ac.correct("TWo", false).has_value());
        REQUIRE(ac.correct("teh", false) == std::optional<std::string>("the"));
    }

    SECTION("Capitalize off: table still applies, uncapped") {
        AutoCorrect ac;
        ac.settings.capitalizeFirstLetterOfSentence = false;
        REQUIRE(ac.correct("teh", true) == std::optional<std::string>("the"));
        REQUIRE_FALSE(ac.correct("hello", true).has_value());
    }
}

TEST_CASE("AutoCorrect::isSentenceStart - String prefix", "[autocorrect]") {
    SECTION("Document start") {
        REQUIRE(AutoCorrect::isSentenceStart(""));
        REQUIRE(AutoCorrect::isSentenceStart("   "));
    }

    SECTION("After sentence terminators") {
        REQUIRE(AutoCorrect::isSentenceStart("Hello. "));
        REQUIRE(AutoCorrect::isSentenceStart("Hello!  "));
        REQUIRE(AutoCorrect::isSentenceStart("Hello? "));
        REQUIRE(AutoCorrect::isSentenceStart("Hello.\n"));
    }

    SECTION("Mid-sentence") {
        REQUIRE_FALSE(AutoCorrect::isSentenceStart("Hello "));
        REQUIRE_FALSE(AutoCorrect::isSentenceStart("Hello world "));
        REQUIRE_FALSE(AutoCorrect::isSentenceStart("Hello, "));
    }

    SECTION("After a paragraph break") {
        REQUIRE(AutoCorrect::isSentenceStart("Hello\n\n"));
        REQUIRE(AutoCorrect::isSentenceStart("Hello\n"));
    }

    SECTION("After a closing quote following a terminator") {
        REQUIRE(AutoCorrect::isSentenceStart("She said \"Hello.\" "));
        REQUIRE_FALSE(AutoCorrect::isSentenceStart("She said \"Hello "));
    }
}

TEST_CASE("AutoCorrect::isSentenceStart - Buffer", "[autocorrect]") {
    TextBuffer buffer;
    buffer.setText("Hello. wrold and hello world");

    SECTION("Word after a period") {
        // "wrold" starts at offset 7.
        REQUIRE(AutoCorrect::isSentenceStart(buffer, 7));
    }

    SECTION("Word mid-sentence") {
        // Second "hello" starts at offset 17.
        REQUIRE_FALSE(AutoCorrect::isSentenceStart(buffer, 17));
    }

    SECTION("Document start") {
        REQUIRE(AutoCorrect::isSentenceStart(buffer, 0));
    }
}

TEST_CASE("AutoCorrect - One-step undo via replaceRange", "[autocorrect]") {
    // The typing hook applies word + delimiter as one ReplaceRangesCommand;
    // one undo must restore the word before correction.
    TextBuffer buffer;
    buffer.setText("teh");
    buffer.replaceRange(0, 3, "the ");
    REQUIRE(buffer.getText() == "the ");
    buffer.undo();
    REQUIRE(buffer.getText() == "teh");
}
