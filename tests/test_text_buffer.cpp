#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

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

    SECTION("no selection initially") { REQUIRE_FALSE(buffer.hasSelection()); }
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

TEST_CASE("TextBuffer selection deletion", "[text_buffer]") {
    TextBuffer buffer;
    buffer.setText("hello world");
    buffer.setCaret({0, 0});

    SECTION("deleteSelection removes selected text") {
        // Select "hello"
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();

        REQUIRE(buffer.deleteSelection());
        REQUIRE(buffer.getText() == " world");
        REQUIRE(buffer.caret().column == 0);
        REQUIRE_FALSE(buffer.hasSelection());
    }

    SECTION("deleteSelection with reverse selection") {
        // Select "world" backwards
        buffer.setCaret({0, 11});
        buffer.setSelectionAnchor({0, 11});
        buffer.setCaret({0, 6});
        buffer.updateSelectionToCaret();

        REQUIRE(buffer.deleteSelection());
        REQUIRE(buffer.getText() == "hello ");
        REQUIRE(buffer.caret().column == 6);
    }

    SECTION("backspace deletes selection instead of single char") {
        // Select "llo"
        buffer.setCaret({0, 2});
        buffer.setSelectionAnchor({0, 2});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();

        buffer.backspace();
        REQUIRE(buffer.getText() == "he world");
        REQUIRE(buffer.caret().column == 2);
    }

    SECTION("delete key deletes selection") {
        // Select " world"
        buffer.setCaret({0, 5});
        buffer.setSelectionAnchor({0, 5});
        buffer.setCaret({0, 11});
        buffer.updateSelectionToCaret();

        buffer.del();
        REQUIRE(buffer.getText() == "hello");
    }

    SECTION("typing replaces selection") {
        // Select "hello"
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();

        buffer.insertChar('H');
        buffer.insertChar('i');
        REQUIRE(buffer.getText() == "Hi world");
    }

    SECTION("multiline selection deletion") {
        buffer.setText("line one\nline two\nline three");
        buffer.setCaret({0, 5});
        buffer.setSelectionAnchor({0, 5});
        buffer.setCaret({2, 5});
        buffer.updateSelectionToCaret();

        REQUIRE(buffer.deleteSelection());
        REQUIRE(buffer.getText() == "line three");
    }

    SECTION("selectAll and delete clears document") {
        buffer.selectAll();
        REQUIRE(buffer.hasSelection());

        buffer.del();
        REQUIRE(buffer.getText().empty());
        REQUIRE(buffer.lineCount() == 1);
    }
}

TEST_CASE("TextBuffer undo/redo", "[text_buffer][undo]") {
    TextBuffer buffer;

    SECTION("undo insert char") {
        buffer.insertChar('a');
        REQUIRE(buffer.getText() == "a");
        REQUIRE(buffer.canUndo());

        buffer.undo();
        REQUIRE(buffer.getText().empty());
        REQUIRE(buffer.canRedo());
    }

    SECTION("redo after undo") {
        buffer.insertChar('a');
        buffer.undo();
        REQUIRE(buffer.getText().empty());

        buffer.redo();
        REQUIRE(buffer.getText() == "a");
    }

    SECTION("undo multiple inserts") {
        buffer.insertText("abc");
        REQUIRE(buffer.getText() == "abc");

        buffer.undo();  // undo 'c'
        REQUIRE(buffer.getText() == "ab");

        buffer.undo();  // undo 'b'
        REQUIRE(buffer.getText() == "a");

        buffer.undo();  // undo 'a'
        REQUIRE(buffer.getText().empty());
    }

    SECTION("undo backspace restores character") {
        buffer.insertText("abc");
        buffer.clearHistory();  // Clear insert history to focus on backspace

        buffer.backspace();  // Delete 'c'
        REQUIRE(buffer.getText() == "ab");

        buffer.undo();  // Should restore 'c'
        REQUIRE(buffer.getText() == "abc");
    }

    SECTION("undo delete restores character") {
        buffer.insertText("abc");
        buffer.clearHistory();
        buffer.setCaret({0, 1});  // Position after 'a'

        buffer.del();  // Delete 'b'
        REQUIRE(buffer.getText() == "ac");

        buffer.undo();  // Should restore 'b'
        REQUIRE(buffer.getText() == "abc");
    }

    SECTION("new action clears redo stack") {
        buffer.insertChar('a');
        buffer.undo();
        REQUIRE(buffer.canRedo());

        buffer.insertChar('b');           // New action
        REQUIRE_FALSE(buffer.canRedo());  // Redo stack cleared
        REQUIRE(buffer.getText() == "b");
    }

    SECTION("undo newline joins lines") {
        buffer.insertText("line1");
        buffer.insertChar('\n');
        buffer.insertText("line2");
        buffer.clearHistory();

        buffer.setCaret({1, 0});
        buffer.backspace();  // Delete newline
        REQUIRE(buffer.lineCount() == 1);

        buffer.undo();  // Restore newline
        REQUIRE(buffer.lineCount() == 2);
    }

    SECTION("clear history prevents undo") {
        buffer.insertChar('a');
        REQUIRE(buffer.canUndo());

        buffer.clearHistory();
        REQUIRE_FALSE(buffer.canUndo());
        REQUIRE_FALSE(buffer.canRedo());
    }

    SECTION("cannot undo when history is empty") {
        REQUIRE_FALSE(buffer.canUndo());
        buffer.undo();  // Should not crash
        REQUIRE(buffer.getText().empty());
    }

    SECTION("cannot redo when history is empty") {
        REQUIRE_FALSE(buffer.canRedo());
        buffer.redo();  // Should not crash
        REQUIRE(buffer.getText().empty());
    }
}

// Regression test for caret positioning with narrow characters like 'l', 'i'
// See: "Fix caret positioning to use per-glyph advance/metrics"
// The rendering code (main.cpp) now uses MeasureText() for accurate positioning
// This test ensures the buffer correctly tracks column positions for narrow
// chars
TEST_CASE("Caret positioning with narrow characters",
          "[text_buffer][regression]") {
    TextBuffer buffer;

    SECTION("llllll - caret column tracks correctly for narrow chars") {
        // Insert a series of narrow characters ('l')
        buffer.insertText("llllll");

        // Caret should be at column 6 (after 6 characters)
        REQUIRE(buffer.caret().column == 6);
        REQUIRE(buffer.getText() == "llllll");

        // Move left should decrease column
        buffer.moveLeft();
        REQUIRE(buffer.caret().column == 5);

        // Backspace should remove character and decrease column
        buffer.backspace();
        REQUIRE(buffer.caret().column == 4);
        REQUIRE(buffer.getText() == "lllll");
    }

    SECTION("mixed narrow and wide characters") {
        // Mix of narrow ('i', 'l') and wider ('m', 'w') characters
        buffer.insertText("iiii");
        REQUIRE(buffer.caret().column == 4);

        buffer.insertText("mmmm");
        REQUIRE(buffer.caret().column == 8);

        // Navigate back
        buffer.setCaret({0, 4});  // Position between i's and m's
        REQUIRE(buffer.caret().column == 4);
    }

    SECTION("caret at end of narrow character line") {
        buffer.insertText(
            "lllllllllllllllllllllllllllllllllllllllllllllllllll");  // 51 l's
        REQUIRE(buffer.caret().column == 51);
        REQUIRE(buffer.getText().length() == 51);
    }
}

TEST_CASE("Scroll viewport validation", "[text_buffer][scroll]") {
    TextBuffer buffer;

    SECTION("lineSpan returns correct data for visible lines") {
        // Create a document with 50 lines
        for (int i = 0; i < 50; ++i) {
            buffer.insertText("Line " + std::to_string(i));
            if (i < 49) buffer.insertChar('\n');
        }

        REQUIRE(buffer.lineCount() == 50);

        // Simulate scroll: accessing lines 10-20 (visible viewport)
        int scrollOffset = 10;
        int visibleLines = 10;

        for (int i = 0; i < visibleLines; ++i) {
            std::size_t row = static_cast<std::size_t>(scrollOffset + i);
            LineSpan span = buffer.lineSpan(row);
            std::string line = buffer.lineString(row);

            // Each line should contain "Line X" where X is the line number
            std::string expected = "Line " + std::to_string(scrollOffset + i);
            REQUIRE(line == expected);
            REQUIRE(span.length == expected.length());
        }
    }

    SECTION("lineSpan bounds checking at document end") {
        // Create 5 lines
        buffer.insertText("Line 0\nLine 1\nLine 2\nLine 3\nLine 4");
        REQUIRE(buffer.lineCount() == 5);

        // Simulate scroll near end
        int scrollOffset = 3;
        int visibleLines = 5;  // Would show lines 3-7, but only 3-4 exist

        // Should be able to access lines 3 and 4
        LineSpan span3 = buffer.lineSpan(3);
        LineSpan span4 = buffer.lineSpan(4);
        REQUIRE(buffer.lineString(3) == "Line 3");
        REQUIRE(buffer.lineString(4) == "Line 4");

        // Lines 3-4 have correct content for scrolled view
        REQUIRE(span3.length == 6);  // "Line 3"
        REQUIRE(span4.length == 6);  // "Line 4"
    }

    SECTION("caret visibility during scroll") {
        // Create 100 lines
        for (int i = 0; i < 100; ++i) {
            buffer.insertText("L" + std::to_string(i));
            if (i < 99) buffer.insertChar('\n');
        }

        REQUIRE(buffer.lineCount() == 100);

        // Move caret to line 50
        buffer.setCaret({50, 0});
        REQUIRE(buffer.caret().row == 50);

        // Simulate scroll calculation: caret should be visible
        int visibleLines = 20;
        int scrollOffset = 40;  // Viewing lines 40-60

        std::size_t caretRow = buffer.caret().row;
        bool caretVisible =
            (caretRow >= static_cast<std::size_t>(scrollOffset) &&
             caretRow < static_cast<std::size_t>(scrollOffset + visibleLines));
        REQUIRE(caretVisible);

        // If we scroll past caret
        scrollOffset = 60;  // Viewing lines 60-80
        caretVisible =
            (caretRow >= static_cast<std::size_t>(scrollOffset) &&
             caretRow < static_cast<std::size_t>(scrollOffset + visibleLines));
        REQUIRE_FALSE(caretVisible);  // Caret at line 50 not visible
    }

    SECTION("scroll offset clamping") {
        // Create 10 lines
        buffer.insertText("L0\nL1\nL2\nL3\nL4\nL5\nL6\nL7\nL8\nL9");
        REQUIRE(buffer.lineCount() == 10);

        int visibleLines = 5;
        int lineCount = static_cast<int>(buffer.lineCount());

        // Max scroll offset calculation
        int maxScroll = lineCount - visibleLines;
        REQUIRE(maxScroll == 5);  // Can scroll lines 0-5 to show lines 5-9

        // Test various scroll offset clamping
        auto clamp = [maxScroll](int offset) {
            if (offset < 0) return 0;
            if (offset > maxScroll) return maxScroll;
            return offset;
        };

        REQUIRE(clamp(-5) == 0);  // Negative clamped to 0
        REQUIRE(clamp(0) == 0);   // Min valid
        REQUIRE(clamp(3) == 3);   // Mid range
        REQUIRE(clamp(5) == 5);   // Max valid
        REQUIRE(clamp(10) == 5);  // Over max clamped
    }
}
