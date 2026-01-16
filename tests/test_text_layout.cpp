#include "catch2/catch.hpp"
#include "../src/editor/text_layout.h"

TEST_CASE("layoutWrappedLines basic wrapping", "[text_layout]") {
    TextBuffer buffer;

    SECTION("empty buffer returns single empty wrapped line") {
        auto wrapped = layoutWrappedLines(buffer, 80);
        REQUIRE(wrapped.size() == 1);
        REQUIRE(wrapped[0].text.empty());
        REQUIRE(wrapped[0].source_row == 0);
    }

    SECTION("zero max_columns returns empty") {
        buffer.setText("hello");
        auto wrapped = layoutWrappedLines(buffer, 0);
        REQUIRE(wrapped.empty());
    }

    SECTION("short line doesn't wrap") {
        buffer.setText("hello");
        auto wrapped = layoutWrappedLines(buffer, 80);
        REQUIRE(wrapped.size() == 1);
        REQUIRE(wrapped[0].text == "hello");
        REQUIRE(wrapped[0].source_row == 0);
        REQUIRE(wrapped[0].start_column == 0);
        REQUIRE(wrapped[0].length == 5);
    }

    SECTION("long line wraps at max_columns") {
        buffer.setText("hello world");
        auto wrapped = layoutWrappedLines(buffer, 5);
        REQUIRE(wrapped.size() == 3);
        
        REQUIRE(wrapped[0].text == "hello");
        REQUIRE(wrapped[0].start_column == 0);
        
        REQUIRE(wrapped[1].text == " worl");
        REQUIRE(wrapped[1].start_column == 5);
        
        REQUIRE(wrapped[2].text == "d");
        REQUIRE(wrapped[2].start_column == 10);
    }

    SECTION("multiple lines wrap independently") {
        buffer.setText("abc\ndefghi");
        auto wrapped = layoutWrappedLines(buffer, 3);
        
        REQUIRE(wrapped.size() == 3);
        REQUIRE(wrapped[0].text == "abc");
        REQUIRE(wrapped[0].source_row == 0);
        
        REQUIRE(wrapped[1].text == "def");
        REQUIRE(wrapped[1].source_row == 1);
        REQUIRE(wrapped[1].start_column == 0);
        
        REQUIRE(wrapped[2].text == "ghi");
        REQUIRE(wrapped[2].source_row == 1);
        REQUIRE(wrapped[2].start_column == 3);
    }

    SECTION("empty lines are preserved") {
        buffer.setText("a\n\nb");
        auto wrapped = layoutWrappedLines(buffer, 80);
        
        REQUIRE(wrapped.size() == 3);
        REQUIRE(wrapped[0].text == "a");
        REQUIRE(wrapped[1].text == "");
        REQUIRE(wrapped[2].text == "b");
    }
}

TEST_CASE("layoutWrappedLinesSoA (SoA layout)", "[text_layout][soa]") {
    TextBuffer buffer;

    SECTION("empty buffer returns single entry") {
        auto result = layoutWrappedLinesSoA(buffer, 80);
        REQUIRE(result.size() == 1);
        REQUIRE(result.source_rows[0] == 0);
        REQUIRE(result.start_columns[0] == 0);
        REQUIRE(result.lengths[0] == 0);
    }

    SECTION("zero max_columns returns empty") {
        buffer.setText("hello");
        auto result = layoutWrappedLinesSoA(buffer, 0);
        REQUIRE(result.empty());
    }

    SECTION("SoA result matches AoS result") {
        buffer.setText("hello world\nfoo bar baz");
        
        auto aos = layoutWrappedLines(buffer, 5);
        auto soa = layoutWrappedLinesSoA(buffer, 5);
        
        REQUIRE(aos.size() == soa.size());
        for (std::size_t i = 0; i < aos.size(); ++i) {
            REQUIRE(aos[i].source_row == soa.source_rows[i]);
            REQUIRE(aos[i].start_column == soa.start_columns[i]);
            REQUIRE(aos[i].length == soa.lengths[i]);
        }
    }

    SECTION("SoA handles empty lines") {
        buffer.setText("a\n\nb");
        auto result = layoutWrappedLinesSoA(buffer, 80);
        
        REQUIRE(result.size() == 3);
        REQUIRE(result.lengths[0] == 1);  // "a"
        REQUIRE(result.lengths[1] == 0);  // empty line
        REQUIRE(result.lengths[2] == 1);  // "b"
    }

    SECTION("SoA avoids string copies (no text field)") {
        // This test verifies the SoA API stores only offsets, not strings
        // The benefit is reduced memory allocation during layout
        buffer.setText("The quick brown fox jumps over the lazy dog");
        auto result = layoutWrappedLinesSoA(buffer, 10);
        
        // 44 chars / 10 = 5 segments (4 full + 1 partial)
        REQUIRE(result.size() == 5);
        
        // All segments should reference row 0
        for (std::size_t i = 0; i < result.size(); ++i) {
            REQUIRE(result.source_rows[i] == 0);
        }
        
        // Verify offsets allow reconstructing the text
        const std::vector<std::string> &lines = buffer.lines();
        REQUIRE_FALSE(lines.empty());
        const std::string &line = lines[0];
        
        std::string reconstructed;
        for (std::size_t i = 0; i < result.size(); ++i) {
            std::size_t start = result.start_columns[i];
            std::size_t len = result.lengths[i];
            // Safety check
            REQUIRE(start <= line.size());
            REQUIRE(start + len <= line.size());
            reconstructed += line.substr(start, len);
        }
        REQUIRE(reconstructed == line);
    }
}
