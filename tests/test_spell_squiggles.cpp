#include "../src/editor/spell_squiggles.h"

#include "catch2/catch.hpp"

// ============================================================================
// SpellSquiggleCache Tests
// ============================================================================

TEST_CASE("SpellSquiggleCache - Finds misspelled words on visible lines",
          "[spellcheck][squiggles]") {
    SpellChecker checker;
    REQUIRE(checker.dictionarySize() > 0);
    TextBuffer buffer;
    buffer.setText("hello wrold");
    SpellSquiggleCache cache;

    SECTION("One squiggle for one typo") {
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.size() == 1);
        REQUIRE(cache.squiggles[0].row == 0);
        REQUIRE(cache.squiggles[0].startCol == 6);
        REQUIRE(cache.squiggles[0].endCol == 11);
    }

    SECTION("Clean text has no squiggles") {
        buffer.setText("hello world");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.empty());
    }

    SECTION("Only visible rows are checked") {
        buffer.setText("hello wrold\nsecond line teh here");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.size() == 1);
        REQUIRE(cache.squiggles[0].row == 0);

        REQUIRE(cache.update(buffer, checker, 1, 2));
        REQUIRE(cache.squiggles.size() == 1);
        REQUIRE(cache.squiggles[0].row == 1);
        REQUIRE(cache.squiggles[0].startCol == 12);
        REQUIRE(cache.squiggles[0].endCol == 15);
    }

    SECTION("Empty document has no squiggles") {
        buffer.setText("");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.empty());
    }
}

TEST_CASE("SpellSquiggleCache - Offset to line mapping",
          "[spellcheck][squiggles]") {
    // The squiggle row/col for an error must agree with the buffer's own
    // offset->position mapping for that error's document offset.
    SpellChecker checker;
    TextBuffer buffer;
    buffer.setText("hello\nwrold here\nthe end");
    SpellSquiggleCache cache;
    REQUIRE(cache.update(buffer, checker, 0, 3));
    REQUIRE(cache.squiggles.size() == 1);

    // "wrold" starts at document offset 6 (after "hello\n").
    CaretPosition pos = buffer.positionForOffset(6);
    REQUIRE(pos.row == 1);
    REQUIRE(pos.column == 0);
    REQUIRE(cache.squiggles[0].row == pos.row);
    REQUIRE(cache.squiggles[0].startCol == pos.column);

    // The second line's other word ("here") is correct: no second squiggle.
    REQUIRE(cache.squiggles[0].endCol == 5);
}

TEST_CASE("SpellSquiggleCache - Invalidates on edit, not per frame",
          "[spellcheck][squiggles]") {
    SpellChecker checker;
    TextBuffer buffer;
    buffer.setText("hello wrold");
    SpellSquiggleCache cache;

    REQUIRE(cache.update(buffer, checker, 0, 1));
    REQUIRE(cache.recheckCount == 1);

    SECTION("Same version + range is a no-op") {
        REQUIRE_FALSE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.recheckCount == 1);
    }

    SECTION("Scroll change rechecks the new range") {
        buffer.setText("hello wrold\nteh here");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.recheckCount == 2);
        REQUIRE(cache.update(buffer, checker, 1, 2));
        REQUIRE(cache.recheckCount == 3);
        REQUIRE(cache.squiggles.size() == 1);
        REQUIRE(cache.squiggles[0].row == 1);
    }

    SECTION("Fixing the typo clears the squiggle") {
        REQUIRE(cache.squiggles.size() == 1);  // From the update above
        buffer.setText("hello world");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.empty());
    }

    SECTION("Ignored words lose their squiggle") {
        REQUIRE(cache.squiggles.size() == 1);  // From the update above
        checker.ignoreWord("wrold");
        // Ignore-list change does not bump the buffer version, so the
        // caller must recheck explicitly; a version bump (any edit)
        // also refreshes. Simulate an edit to force the recheck.
        buffer.insertText(" ");
        REQUIRE(cache.update(buffer, checker, 0, 1));
        REQUIRE(cache.squiggles.empty());
    }
}

TEST_CASE("rawColumnToDisplayColumn - Tab expansion",
          "[spellcheck][squiggles]") {
    SECTION("No tabs") {
        const char* line = "wrold here";
        REQUIRE(rawColumnToDisplayColumn(line, 10, 0, 4) == 0);
        REQUIRE(rawColumnToDisplayColumn(line, 10, 6, 4) == 6);
    }

    SECTION("Tab before the word shifts the display column") {
        // "a\twrold": 'a' at 0, tab expands to col 4, "wrold" starts at 4.
        const char* line = "a\twrold";
        REQUIRE(rawColumnToDisplayColumn(line, 7, 2, 4) == 4);
        REQUIRE(rawColumnToDisplayColumn(line, 7, 7, 4) == 9);
    }

    SECTION("Tab width 0 disables expansion") {
        const char* line = "a\twrold";
        REQUIRE(rawColumnToDisplayColumn(line, 7, 2, 0) == 2);
    }
}
