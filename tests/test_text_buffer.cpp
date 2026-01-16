#include "catch2/catch.hpp"
#include "../src/editor/text_buffer.h"

TEST_CASE("TextBuffer initialization", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("starts with one empty line") {
        REQUIRE(buffer.lines().size() == 1);
        REQUIRE(buffer.lines()[0].empty());
    }

    SECTION("caret starts at origin") {
        CaretPosition caret = buffer.caret();
        REQUIRE(caret.row == 0);
        REQUIRE(caret.column == 0);
    }

    SECTION("no selection initially") {
        REQUIRE_FALSE(buffer.hasSelection());
    }
}

TEST_CASE("TextBuffer insert operations", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("insert single character") {
        buffer.insertChar('a');
        REQUIRE(buffer.getText() == "a");
        REQUIRE(buffer.caret().column == 1);
    }

    SECTION("insert multiple characters") {
        buffer.insertChar('h');
        buffer.insertChar('i');
        REQUIRE(buffer.getText() == "hi");
        REQUIRE(buffer.caret().column == 2);
    }

    SECTION("insert newline creates new line") {
        buffer.insertChar('a');
        buffer.insertChar('\n');
        buffer.insertChar('b');
        REQUIRE(buffer.lines().size() == 2);
        REQUIRE(buffer.lines()[0] == "a");
        REQUIRE(buffer.lines()[1] == "b");
    }

    SECTION("insertText works for multichar strings") {
        buffer.insertText("hello");
        REQUIRE(buffer.getText() == "hello");
    }
}

TEST_CASE("TextBuffer setText and getText", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("setText replaces content") {
        buffer.insertChar('x');
        buffer.setText("new content");
        REQUIRE(buffer.getText() == "new content");
    }

    SECTION("setText handles newlines") {
        buffer.setText("line1\nline2\nline3");
        REQUIRE(buffer.lines().size() == 3);
        REQUIRE(buffer.lines()[0] == "line1");
        REQUIRE(buffer.lines()[1] == "line2");
        REQUIRE(buffer.lines()[2] == "line3");
    }

    SECTION("setText handles Windows line endings") {
        buffer.setText("line1\r\nline2");
        REQUIRE(buffer.lines().size() == 2);
        REQUIRE(buffer.lines()[0] == "line1");
        REQUIRE(buffer.lines()[1] == "line2");
    }

    SECTION("getText round-trips correctly") {
        std::string original = "line1\nline2\nline3";
        buffer.setText(original);
        REQUIRE(buffer.getText() == original);
    }
}

TEST_CASE("TextBuffer backspace operations", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("backspace at start does nothing") {
        buffer.backspace();
        REQUIRE(buffer.getText().empty());
    }

    SECTION("backspace deletes previous character") {
        buffer.insertText("ab");
        buffer.backspace();
        REQUIRE(buffer.getText() == "a");
    }

    SECTION("backspace at line start merges lines") {
        buffer.setText("a\nb");
        buffer.setCaret({1, 0});
        buffer.backspace();
        REQUIRE(buffer.lines().size() == 1);
        REQUIRE(buffer.getText() == "ab");
    }
}

TEST_CASE("TextBuffer delete operations", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("delete at end does nothing") {
        buffer.insertChar('a');
        buffer.del();
        REQUIRE(buffer.getText() == "a");
    }

    SECTION("delete removes next character") {
        buffer.insertText("ab");
        buffer.setCaret({0, 0});
        buffer.del();
        REQUIRE(buffer.getText() == "b");
    }

    SECTION("delete at line end merges with next line") {
        buffer.setText("a\nb");
        buffer.setCaret({0, 1});
        buffer.del();
        REQUIRE(buffer.lines().size() == 1);
        REQUIRE(buffer.getText() == "ab");
    }
}

TEST_CASE("TextBuffer caret movement", "[text_buffer]") {
    TextBuffer buffer;
    buffer.setText("abc\ndef");

    SECTION("moveLeft at start stays at start") {
        buffer.setCaret({0, 0});
        buffer.moveLeft();
        REQUIRE(buffer.caret().row == 0);
        REQUIRE(buffer.caret().column == 0);
    }

    SECTION("moveLeft at line start goes to previous line end") {
        buffer.setCaret({1, 0});
        buffer.moveLeft();
        REQUIRE(buffer.caret().row == 0);
        REQUIRE(buffer.caret().column == 3);
    }

    SECTION("moveRight at end stays at end") {
        buffer.setCaret({1, 3});
        buffer.moveRight();
        REQUIRE(buffer.caret().row == 1);
        REQUIRE(buffer.caret().column == 3);
    }

    SECTION("moveRight at line end goes to next line start") {
        buffer.setCaret({0, 3});
        buffer.moveRight();
        REQUIRE(buffer.caret().row == 1);
        REQUIRE(buffer.caret().column == 0);
    }

    SECTION("moveUp at first line stays at first line") {
        buffer.setCaret({0, 1});
        buffer.moveUp();
        REQUIRE(buffer.caret().row == 0);
    }

    SECTION("moveUp goes to previous line") {
        buffer.setCaret({1, 1});
        buffer.moveUp();
        REQUIRE(buffer.caret().row == 0);
        REQUIRE(buffer.caret().column == 1);
    }

    SECTION("moveUp clamps column to line length") {
        buffer.setText("a\nlong");
        buffer.setCaret({1, 4});
        buffer.moveUp();
        REQUIRE(buffer.caret().row == 0);
        REQUIRE(buffer.caret().column == 1);  // clamped to "a" length
    }

    SECTION("moveDown at last line stays at last line") {
        buffer.setCaret({1, 1});
        buffer.moveDown();
        REQUIRE(buffer.caret().row == 1);
    }

    SECTION("moveDown goes to next line") {
        buffer.setCaret({0, 1});
        buffer.moveDown();
        REQUIRE(buffer.caret().row == 1);
        REQUIRE(buffer.caret().column == 1);
    }
}

TEST_CASE("TextBuffer selection", "[text_buffer]") {
    TextBuffer buffer;
    buffer.setText("hello world");

    SECTION("setSelectionAnchor enables selection") {
        buffer.setCaret({0, 0});
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        REQUIRE(buffer.hasSelection());
    }

    SECTION("clearSelection disables selection") {
        buffer.setSelectionAnchor({0, 0});
        buffer.clearSelection();
        REQUIRE_FALSE(buffer.hasSelection());
    }

    SECTION("selectionStart and selectionEnd are ordered") {
        buffer.setCaret({0, 5});
        buffer.setSelectionAnchor({0, 5});
        buffer.setCaret({0, 0});
        buffer.updateSelectionToCaret();

        CaretPosition start = buffer.selectionStart();
        CaretPosition end = buffer.selectionEnd();
        REQUIRE(start.column == 0);
        REQUIRE(end.column == 5);
    }
}

TEST_CASE("TextBuffer text style", "[text_buffer]") {
    TextBuffer buffer;

    SECTION("default style") {
        TextStyle style = buffer.textStyle();
        REQUIRE_FALSE(style.bold);
        REQUIRE_FALSE(style.italic);
        REQUIRE(style.font == "Gaegu-Bold");
    }

    SECTION("setTextStyle updates style") {
        TextStyle style;
        style.bold = true;
        style.italic = true;
        style.font = "Arial";
        buffer.setTextStyle(style);

        TextStyle result = buffer.textStyle();
        REQUIRE(result.bold);
        REQUIRE(result.italic);
        REQUIRE(result.font == "Arial");
    }
}
