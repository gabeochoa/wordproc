#include "catch2/catch.hpp"

#include <afterhours/src/plugins/ui/text_input/state.h>
#include <afterhours/src/plugins/ui/text_input/utils.h>

using namespace afterhours::text_input;

// ============================================================
// UTF-8 Utility Tests
// ============================================================

TEST_CASE("utf8_char_length returns correct byte counts", "[text_input][utf8]") {
    SECTION("ASCII characters are 1 byte") {
        std::string s = "Hello";
        REQUIRE(utf8_char_length(s, 0) == 1);
        REQUIRE(utf8_char_length(s, 4) == 1);
    }

    SECTION("2-byte sequences") {
        std::string s = "\xC3\xA9";  // é
        REQUIRE(utf8_char_length(s, 0) == 2);
    }

    SECTION("3-byte sequences (CJK)") {
        std::string s = "\xE4\xB8\xAD";  // 中
        REQUIRE(utf8_char_length(s, 0) == 3);
    }

    SECTION("4-byte sequences (emoji)") {
        std::string s = "\xF0\x9F\x98\x80";  // 😀
        REQUIRE(utf8_char_length(s, 0) == 4);
    }

    SECTION("past end returns 0") {
        std::string s = "x";
        REQUIRE(utf8_char_length(s, 5) == 0);
    }
}

TEST_CASE("utf8_prev_char_start finds previous character boundary", "[text_input][utf8]") {
    SECTION("ASCII string") {
        std::string s = "abc";
        REQUIRE(utf8_prev_char_start(s, 3) == 2);
        REQUIRE(utf8_prev_char_start(s, 2) == 1);
        REQUIRE(utf8_prev_char_start(s, 1) == 0);
    }

    SECTION("multi-byte character") {
        std::string s = "a\xC3\xA9";  // "aé"
        REQUIRE(utf8_prev_char_start(s, 3) == 1);
        REQUIRE(utf8_prev_char_start(s, 1) == 0);
    }

    SECTION("at start returns 0") {
        std::string s = "hello";
        REQUIRE(utf8_prev_char_start(s, 0) == 0);
    }

    SECTION("empty string returns 0") {
        std::string s;
        REQUIRE(utf8_prev_char_start(s, 0) == 0);
    }
}

TEST_CASE("utf8_to_codepoint decodes correctly", "[text_input][utf8]") {
    SECTION("ASCII") {
        std::string s = "A";
        REQUIRE(utf8_to_codepoint(s, 0) == 0x41);
    }

    SECTION("2-byte (é = U+00E9)") {
        std::string s = "\xC3\xA9";
        REQUIRE(utf8_to_codepoint(s, 0) == 0xE9);
    }

    SECTION("3-byte (中 = U+4E2D)") {
        std::string s = "\xE4\xB8\xAD";
        REQUIRE(utf8_to_codepoint(s, 0) == 0x4E2D);
    }

    SECTION("4-byte (😀 = U+1F600)") {
        std::string s = "\xF0\x9F\x98\x80";
        REQUIRE(utf8_to_codepoint(s, 0) == 0x1F600);
    }

    SECTION("past end returns 0") {
        std::string s = "x";
        REQUIRE(utf8_to_codepoint(s, 5) == 0);
    }
}

TEST_CASE("codepoint_to_utf8 encodes correctly", "[text_input][utf8]") {
    SECTION("ASCII") {
        REQUIRE(codepoint_to_utf8(0x41) == "A");
    }

    SECTION("2-byte") {
        REQUIRE(codepoint_to_utf8(0xE9) == "\xC3\xA9");
    }

    SECTION("3-byte") {
        REQUIRE(codepoint_to_utf8(0x4E2D) == "\xE4\xB8\xAD");
    }

    SECTION("4-byte") {
        REQUIRE(codepoint_to_utf8(0x1F600) == "\xF0\x9F\x98\x80");
    }

    SECTION("negative codepoint returns empty") {
        REQUIRE(codepoint_to_utf8(-1).empty());
    }

    SECTION("roundtrip encode-decode") {
        for (int cp : {0x41, 0xE9, 0x4E2D, 0x1F600}) {
            std::string encoded = codepoint_to_utf8(cp);
            REQUIRE(utf8_to_codepoint(encoded, 0) == cp);
        }
    }
}

// ============================================================
// HasTextInputState Insert/Delete Tests
// ============================================================

TEST_CASE("insert_char inserts ASCII characters", "[text_input][insert]") {
    HasTextInputState state;

    REQUIRE(insert_char(state, 'H'));
    REQUIRE(insert_char(state, 'i'));
    REQUIRE(state.text() == "Hi");
    REQUIRE(state.cursor_position == 2);
    REQUIRE(state.changed_since == true);
}

TEST_CASE("insert_char inserts UTF-8 multi-byte characters", "[text_input][insert]") {
    HasTextInputState state;

    REQUIRE(insert_char(state, 0xE9));  // é
    REQUIRE(state.text() == "\xC3\xA9");
    REQUIRE(state.cursor_position == 2);  // 2 bytes
}

TEST_CASE("insert_char rejects control characters", "[text_input][insert]") {
    HasTextInputState state;

    REQUIRE_FALSE(insert_char(state, 0));    // NUL
    REQUIRE_FALSE(insert_char(state, 10));   // newline
    REQUIRE_FALSE(insert_char(state, 27));   // ESC
    REQUIRE(state.text().empty());
}

TEST_CASE("insert_char allows tab", "[text_input][insert]") {
    HasTextInputState state;
    REQUIRE(insert_char(state, '\t'));
    REQUIRE(state.text() == "\t");
}

TEST_CASE("insert_char respects max_length", "[text_input][insert]") {
    HasTextInputState state("", 3);

    REQUIRE(insert_char(state, 'a'));
    REQUIRE(insert_char(state, 'b'));
    REQUIRE(insert_char(state, 'c'));
    REQUIRE_FALSE(insert_char(state, 'd'));
    REQUIRE(state.text() == "abc");
}

TEST_CASE("delete_before_cursor removes last character", "[text_input][delete]") {
    HasTextInputState state;
    insert_char(state, 'a');
    insert_char(state, 'b');
    insert_char(state, 'c');

    REQUIRE(delete_before_cursor(state));
    REQUIRE(state.text() == "ab");
    REQUIRE(state.cursor_position == 2);
}

TEST_CASE("delete_before_cursor handles multi-byte characters", "[text_input][delete]") {
    HasTextInputState state;
    insert_char(state, 0x4E2D);  // 中 (3 bytes)
    REQUIRE(state.cursor_position == 3);

    REQUIRE(delete_before_cursor(state));
    REQUIRE(state.text().empty());
    REQUIRE(state.cursor_position == 0);
}

TEST_CASE("delete_before_cursor at position 0 does nothing", "[text_input][delete]") {
    HasTextInputState state;
    REQUIRE_FALSE(delete_before_cursor(state));
}

TEST_CASE("delete_at_cursor removes character at cursor", "[text_input][delete]") {
    HasTextInputState state;
    insert_char(state, 'a');
    insert_char(state, 'b');
    insert_char(state, 'c');
    state.cursor_position = 1;

    REQUIRE(delete_at_cursor(state));
    REQUIRE(state.text() == "ac");
}

TEST_CASE("delete_at_cursor at end does nothing", "[text_input][delete]") {
    HasTextInputState state;
    insert_char(state, 'x');

    REQUIRE_FALSE(delete_at_cursor(state));
    REQUIRE(state.text() == "x");
}

// ============================================================
// Cursor Movement Tests
// ============================================================

TEST_CASE("move_cursor_left moves back one character", "[text_input][cursor]") {
    HasTextInputState state;
    insert_char(state, 'a');
    insert_char(state, 'b');
    REQUIRE(state.cursor_position == 2);

    move_cursor_left(state);
    REQUIRE(state.cursor_position == 1);

    move_cursor_left(state);
    REQUIRE(state.cursor_position == 0);
}

TEST_CASE("move_cursor_left at 0 stays at 0", "[text_input][cursor]") {
    HasTextInputState state;
    move_cursor_left(state);
    REQUIRE(state.cursor_position == 0);
}

TEST_CASE("move_cursor_right moves forward one character", "[text_input][cursor]") {
    HasTextInputState state;
    insert_char(state, 'a');
    insert_char(state, 'b');
    state.cursor_position = 0;

    move_cursor_right(state);
    REQUIRE(state.cursor_position == 1);

    move_cursor_right(state);
    REQUIRE(state.cursor_position == 2);
}

TEST_CASE("move_cursor_right at end stays at end", "[text_input][cursor]") {
    HasTextInputState state;
    insert_char(state, 'x');

    move_cursor_right(state);
    REQUIRE(state.cursor_position == 1);
}

TEST_CASE("cursor movement handles multi-byte UTF-8", "[text_input][cursor]") {
    HasTextInputState state;
    insert_char(state, 0x4E2D);  // 中 (3 bytes)
    insert_char(state, 0x6587);  // 文 (3 bytes)
    REQUIRE(state.cursor_position == 6);

    move_cursor_left(state);
    REQUIRE(state.cursor_position == 3);

    move_cursor_left(state);
    REQUIRE(state.cursor_position == 0);

    move_cursor_right(state);
    REQUIRE(state.cursor_position == 3);
}

// ============================================================
// Selection Tests
// ============================================================

TEST_CASE("HasTextInputState selection basics", "[text_input][selection]") {
    HasTextInputState state("Hello World");
    state.cursor_position = 5;

    SECTION("no selection initially") {
        REQUIRE_FALSE(state.has_selection());
        REQUIRE(state.selected_text().empty());
    }

    SECTION("setting anchor creates selection") {
        state.selection_anchor = 0;
        REQUIRE(state.has_selection());
        REQUIRE(state.selection_start() == 0);
        REQUIRE(state.selection_end() == 5);
        REQUIRE(state.selected_text() == "Hello");
    }

    SECTION("anchor == cursor means no selection") {
        state.selection_anchor = 5;
        REQUIRE_FALSE(state.has_selection());
    }

    SECTION("clear_selection removes anchor") {
        state.selection_anchor = 0;
        REQUIRE(state.has_selection());
        state.clear_selection();
        REQUIRE_FALSE(state.has_selection());
    }

    SECTION("reverse selection (anchor after cursor)") {
        state.cursor_position = 0;
        state.selection_anchor = 5;
        REQUIRE(state.selection_start() == 0);
        REQUIRE(state.selection_end() == 5);
        REQUIRE(state.selected_text() == "Hello");
    }
}

TEST_CASE("delete_selection removes selected text", "[text_input][selection]") {
    HasTextInputState state("Hello World");
    state.cursor_position = 5;
    state.selection_anchor = 0;

    REQUIRE(delete_selection(state));
    REQUIRE(state.text() == " World");
    REQUIRE(state.cursor_position == 0);
    REQUIRE_FALSE(state.has_selection());
    REQUIRE(state.changed_since);
}

TEST_CASE("delete_selection with no selection returns false", "[text_input][selection]") {
    HasTextInputState state("Hello");
    state.cursor_position = 3;

    REQUIRE_FALSE(delete_selection(state));
    REQUIRE(state.text() == "Hello");
}

TEST_CASE("insert_char replaces selection when delete_selection called first", "[text_input][selection]") {
    HasTextInputState state("Hello World");
    state.cursor_position = 5;
    state.selection_anchor = 0;

    delete_selection(state);
    insert_char(state, 'G');
    insert_char(state, 'o');
    REQUIRE(state.text() == "Go World");
}

// ============================================================
// Blink Timer Tests
// ============================================================

TEST_CASE("update_blink toggles cursor visibility", "[text_input][blink]") {
    HasTextInputState state;
    state.cursor_blink_rate = 0.5f;

    SECTION("visible at start") {
        REQUIRE(update_blink(state, 0.0f));
    }

    SECTION("still visible before half-cycle") {
        REQUIRE(update_blink(state, 0.4f));
    }

    SECTION("invisible after half-cycle") {
        REQUIRE_FALSE(update_blink(state, 0.6f));
    }

    SECTION("wraps back to visible after full cycle") {
        state.cursor_blink_timer = 0.95f;
        bool visible = update_blink(state, 0.1f);
        REQUIRE(visible);
    }
}

TEST_CASE("reset_blink resets timer to 0", "[text_input][blink]") {
    HasTextInputState state;
    state.cursor_blink_timer = 0.7f;

    reset_blink(state);
    REQUIRE(state.cursor_blink_timer == 0.0f);
    REQUIRE(update_blink(state, 0.0f));
}

// ============================================================
// Word Navigation Tests
// ============================================================

TEST_CASE("find_word_start finds beginning of word", "[text_input][word]") {
    std::string text = "hello world foo";

    REQUIRE(find_word_start(text, 7) == 6);   // middle of "world" -> start of "world"
    REQUIRE(find_word_start(text, 5) == 0);   // at space after "hello" -> start of "hello"
    REQUIRE(find_word_start(text, 0) == 0);   // already at start
}

TEST_CASE("find_word_end finds end of word", "[text_input][word]") {
    std::string text = "hello world foo";

    REQUIRE(find_word_end(text, 0) == 5);     // start of "hello" -> end
    REQUIRE(find_word_end(text, 5) == 11);    // at space -> end of "world"
    REQUIRE(find_word_end(text, 15) == 15);   // at end stays at end
}

TEST_CASE("select_word_at selects word boundaries", "[text_input][word]") {
    std::string text = "hello world";

    SECTION("selecting in 'hello'") {
        auto [start, end] = select_word_at(text, 2);
        REQUIRE(start == 0);
        REQUIRE(end == 5);
    }

    SECTION("selecting in 'world'") {
        auto [start, end] = select_word_at(text, 8);
        REQUIRE(start == 6);
        REQUIRE(end == 11);
    }

    SECTION("selecting a separator") {
        auto [start, end] = select_word_at(text, 5);
        REQUIRE(start == 5);
        REQUIRE(end == 6);
    }

    SECTION("empty string") {
        auto [start, end] = select_word_at("", 0);
        REQUIRE(start == 0);
        REQUIRE(end == 0);
    }
}

// ============================================================
// CJK Detection Tests
// ============================================================

TEST_CASE("contains_cjk detects CJK characters", "[text_input][cjk]") {
    REQUIRE_FALSE(contains_cjk(""));
    REQUIRE_FALSE(contains_cjk("Hello World"));
    REQUIRE_FALSE(contains_cjk("café"));
    REQUIRE(contains_cjk("\xE4\xB8\xAD"));      // 中
    REQUIRE(contains_cjk("Hello \xE4\xB8\xAD")); // mixed
}

// ============================================================
// Integration: insert at cursor mid-text
// ============================================================

TEST_CASE("insert_char at middle of text", "[text_input][insert]") {
    HasTextInputState state("ac");
    state.cursor_position = 1;

    insert_char(state, 'b');
    REQUIRE(state.text() == "abc");
    REQUIRE(state.cursor_position == 2);
}

TEST_CASE("delete_before_cursor mid-text", "[text_input][delete]") {
    HasTextInputState state("abc");
    state.cursor_position = 2;

    delete_before_cursor(state);
    REQUIRE(state.text() == "ac");
    REQUIRE(state.cursor_position == 1);
}

TEST_CASE("insert after selection delete collapses correctly", "[text_input][selection]") {
    HasTextInputState state("ABCDEF");
    state.selection_anchor = 1;
    state.cursor_position = 4;

    delete_selection(state);
    REQUIRE(state.text() == "AEF");
    REQUIRE(state.cursor_position == 1);

    insert_char(state, 'X');
    REQUIRE(state.text() == "AXEF");
    REQUIRE(state.cursor_position == 2);
}
