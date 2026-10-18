#include "../src/editor/spell_session.h"
#include "../src/editor/text_buffer.h"

#include "catch2/catch.hpp"

// ============================================================================
// findNextSpellingError Tests
// ============================================================================

TEST_CASE("findNextSpellingError - Finds errors from an offset",
          "[spellcheck][spelling-dialog]") {
    SpellChecker checker;
    REQUIRE(checker.dictionarySize() > 0);
    const std::string text = "hello wrold and teh world";

    SECTION("Finds the first error from the start") {
        auto error = findNextSpellingError(checker, text, 0);
        REQUIRE(error.has_value());
        REQUIRE(error->word == "wrold");
        REQUIRE(error->offset == 6);
        REQUIRE(error->length == 5);
        REQUIRE_FALSE(error->suggestions.empty());
    }

    SECTION("Skips errors ending at or before the offset") {
        auto error = findNextSpellingError(checker, text, 11);
        REQUIRE(error.has_value());
        REQUIRE(error->word == "teh");
        REQUIRE(error->offset == 16);
    }

    SECTION("Caret inside a word still checks that word") {
        auto error = findNextSpellingError(checker, text, 8);
        REQUIRE(error.has_value());
        REQUIRE(error->word == "wrold");
    }

    SECTION("Clean remainder returns nullopt") {
        auto error = findNextSpellingError(checker, text, 19);
        REQUIRE_FALSE(error.has_value());
    }

    SECTION("Clean text returns nullopt") {
        auto error = findNextSpellingError(checker, "hello world", 0);
        REQUIRE_FALSE(error.has_value());
    }

    SECTION("Suggestions are real words") {
        auto error = findNextSpellingError(checker, "hello wrold", 0);
        REQUIRE(error.has_value());
        for (const auto& suggestion : error->suggestions) {
            REQUIRE(checker.isCorrect(suggestion));
        }
    }

    SECTION("First suggestion for 'wrold' is 'world' (pinned: E2E relies on it)") {
        // Damerau ranking puts "world" first (one adjacent transposition);
        // the Spelling dialog E2E clicks the first suggestion, so this
        // pins that coupling.
        auto error = findNextSpellingError(checker, "hello wrold", 0);
        REQUIRE(error.has_value());
        REQUIRE_FALSE(error->suggestions.empty());
        REQUIRE(error->suggestions[0] == "world");
    }
}

// ============================================================================
// TextBuffer::replaceRange / replaceRanges Tests (single undo step)
// ============================================================================

TEST_CASE("TextBuffer::replaceRange - Single replace is one undo step",
          "[text-buffer][spelling-dialog]") {
    TextBuffer buffer;
    buffer.setText("hello wrold");

    SECTION("Replace + one undo restores the original") {
        buffer.replaceRange(6, 5, "world");
        REQUIRE(buffer.getText() == "hello world");
        buffer.undo();
        REQUIRE(buffer.getText() == "hello wrold");
    }

    SECTION("Redo re-applies the replacement") {
        buffer.replaceRange(6, 5, "world");
        buffer.undo();
        buffer.redo();
        REQUIRE(buffer.getText() == "hello world");
    }

    SECTION("Replacement + delimiter is still one undo step") {
        buffer.setText("teh");
        buffer.replaceRange(0, 3, "the ");
        REQUIRE(buffer.getText() == "the ");
        buffer.undo();
        REQUIRE(buffer.getText() == "teh");
    }

    SECTION("Out-of-bounds range is a no-op") {
        REQUIRE_FALSE(buffer.canUndo());
        buffer.replaceRange(100, 5, "world");
        REQUIRE(buffer.getText() == "hello wrold");
        REQUIRE_FALSE(buffer.canUndo());
        buffer.replaceRange(6, 100, "world");
        REQUIRE(buffer.getText() == "hello wrold");
        REQUIRE_FALSE(buffer.canUndo());
    }

    SECTION("Empty replacement deletes the range") {
        buffer.replaceRange(6, 5, "");
        REQUIRE(buffer.getText() == "hello ");
        buffer.undo();
        REQUIRE(buffer.getText() == "hello wrold");
    }
}

TEST_CASE("TextBuffer::replaceRanges - Change All is one undo step",
          "[text-buffer][spelling-dialog]") {
    TextBuffer buffer;
    buffer.setText("wrold and wrold");

    SECTION("All occurrences replaced, one undo restores all") {
        FindOptions options;
        options.caseSensitive = false;
        options.wholeWord = true;
        std::vector<TextReplacement> replacements;
        for (const FindResult& match : buffer.findAll("wrold", options)) {
            replacements.push_back({match.start, "wrold", "world"});
        }
        REQUIRE(replacements.size() == 2);
        buffer.replaceRanges(std::move(replacements));
        REQUIRE(buffer.getText() == "world and world");
        buffer.undo();
        REQUIRE(buffer.getText() == "wrold and wrold");
    }

    SECTION("Unsorted ranges are applied correctly") {
        // Deliberately descending: replaceRanges sorts ascending.
        buffer.replaceRanges({
            {{0, 10}, "wrold", "world"},
            {{0, 0}, "wrold", "world"},
        });
        REQUIRE(buffer.getText() == "world and world");
        buffer.undo();
        REQUIRE(buffer.getText() == "wrold and wrold");
    }

    SECTION("Empty replacement list is a no-op") {
        REQUIRE_FALSE(buffer.canUndo());
        buffer.replaceRanges({});
        REQUIRE_FALSE(buffer.canUndo());
    }
}
