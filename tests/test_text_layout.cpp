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
