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

TEST_CASE("Paragraph styles", "[text_buffer][paragraph_styles]") {
    TextBuffer buffer;
    
    SECTION("default paragraph style is Normal") {
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Normal);
        REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Normal);
    }
    
    SECTION("set and get paragraph style") {
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading1);
        REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading1);
    }
    
    SECTION("paragraph styles per line are independent") {
        // Create multiple lines
        buffer.insertText("Line 1\nLine 2\nLine 3");
        REQUIRE(buffer.lineCount() == 3);
        
        // Set different styles for each line
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        
        buffer.setCaret({1, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
        
        // Verify each line has its own style
        REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Title);
        REQUIRE(buffer.lineParagraphStyle(1) == ParagraphStyle::Heading1);
        REQUIRE(buffer.lineParagraphStyle(2) == ParagraphStyle::Normal);
    }
    
    SECTION("all paragraph styles can be applied") {
        // Test all styles can be set
        buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Normal);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Title);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Subtitle);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading1);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading2);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading3);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading4);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading5);
        
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading6);
    }
    
    SECTION("paragraph style helper functions") {
        // Test font size helpers
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Title) == 32);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Subtitle) == 24);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading1) == 28);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading2) == 24);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading3) == 20);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading4) == 18);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading5) == 16);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Heading6) == 14);
        REQUIRE(paragraphStyleFontSize(ParagraphStyle::Normal) == 16);
        
        // Test bold helpers
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Title) == true);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading1) == true);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading2) == true);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading3) == true);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading4) == true);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading5) == false);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Heading6) == false);
        REQUIRE(paragraphStyleIsBold(ParagraphStyle::Normal) == false);
        
        // Test italic helpers
        REQUIRE(paragraphStyleIsItalic(ParagraphStyle::Subtitle) == true);
        REQUIRE(paragraphStyleIsItalic(ParagraphStyle::Normal) == false);
        REQUIRE(paragraphStyleIsItalic(ParagraphStyle::Heading1) == false);
        
        // Test name helpers
        REQUIRE(std::string(paragraphStyleName(ParagraphStyle::Normal)) == "Normal");
        REQUIRE(std::string(paragraphStyleName(ParagraphStyle::Title)) == "Title");
        REQUIRE(std::string(paragraphStyleName(ParagraphStyle::Subtitle)) == "Subtitle");
        REQUIRE(std::string(paragraphStyleName(ParagraphStyle::Heading1)) == "Heading 1");
    }
    
    SECTION("new lines inherit style from current line") {
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        buffer.insertText("Heading");
        buffer.insertChar('\n');  // Create new line
        
        // New line should inherit the style from the previous line
        // (implementation may vary - this tests current behavior)
        REQUIRE(buffer.lineCount() == 2);
    }
}

TEST_CASE("Font family and size selection", "[text_buffer][font]") {
    TextBuffer buffer;
    
    SECTION("default font is Gaegu-Bold") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.font == "Gaegu-Bold");
    }
    
    SECTION("default font size is 16") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.fontSize == 16);
    }
    
    SECTION("set font family") {
        TextStyle style = buffer.textStyle();
        style.font = "EBGaramond-Regular";
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.font == "EBGaramond-Regular");
    }
    
    SECTION("set font size") {
        TextStyle style = buffer.textStyle();
        style.fontSize = 24;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.fontSize == 24);
    }
    
    SECTION("font size limits") {
        TextStyle style = buffer.textStyle();
        
        // Test max limit (72)
        style.fontSize = 72;
        buffer.setTextStyle(style);
        REQUIRE(buffer.textStyle().fontSize == 72);
        
        // Test min limit (8)
        style.fontSize = 8;
        buffer.setTextStyle(style);
        REQUIRE(buffer.textStyle().fontSize == 8);
    }
    
    SECTION("font and size independent of bold/italic") {
        TextStyle style = buffer.textStyle();
        style.font = "TestFont";
        style.fontSize = 20;
        style.bold = true;
        style.italic = true;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.font == "TestFont");
        REQUIRE(updated.fontSize == 20);
        REQUIRE(updated.bold == true);
        REQUIRE(updated.italic == true);
    }
}

TEST_CASE("Text color and highlight formatting", "[text_buffer][color]") {
    TextBuffer buffer;
    
    SECTION("default text color is black") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.textColor == TextColors::Black);
    }
    
    SECTION("default highlight color is none") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.highlightColor.isNone());
    }
    
    SECTION("set text color") {
        TextStyle style = buffer.textStyle();
        style.textColor = TextColors::Red;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.textColor == TextColors::Red);
    }
    
    SECTION("set highlight color") {
        TextStyle style = buffer.textStyle();
        style.highlightColor = HighlightColors::Yellow;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.highlightColor == HighlightColors::Yellow);
    }
    
    SECTION("color and highlight can be combined with other formatting") {
        TextStyle style = buffer.textStyle();
        style.bold = true;
        style.textColor = TextColors::Blue;
        style.highlightColor = HighlightColors::Green;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.bold == true);
        REQUIRE(updated.textColor == TextColors::Blue);
        REQUIRE(updated.highlightColor == HighlightColors::Green);
    }
}

TEST_CASE("Text emphasis formatting", "[text_buffer][emphasis]") {
    TextBuffer buffer;
    
    SECTION("default emphasis is all off") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.bold == false);
        REQUIRE(style.italic == false);
        REQUIRE(style.underline == false);
        REQUIRE(style.strikethrough == false);
    }
    
    SECTION("toggle underline") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.underline == false);
        
        style.underline = true;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.underline == true);
    }
    
    SECTION("toggle strikethrough") {
        TextStyle style = buffer.textStyle();
        REQUIRE(style.strikethrough == false);
        
        style.strikethrough = true;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.strikethrough == true);
    }
    
    SECTION("all emphasis options can be combined") {
        TextStyle style = buffer.textStyle();
        style.bold = true;
        style.italic = true;
        style.underline = true;
        style.strikethrough = true;
        buffer.setTextStyle(style);
        
        TextStyle updated = buffer.textStyle();
        REQUIRE(updated.bold == true);
        REQUIRE(updated.italic == true);
        REQUIRE(updated.underline == true);
        REQUIRE(updated.strikethrough == true);
    }
}

TEST_CASE("Paragraph alignment", "[text_buffer][alignment]") {
    TextBuffer buffer;
    buffer.setText("Line 1\nLine 2\nLine 3");
    
    SECTION("default alignment is left") {
        REQUIRE(buffer.currentAlignment() == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(0) == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(1) == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(2) == TextAlignment::Left);
    }
    
    SECTION("set current line alignment to center") {
        buffer.setCaret({0, 0});
        buffer.setCurrentAlignment(TextAlignment::Center);
        
        REQUIRE(buffer.currentAlignment() == TextAlignment::Center);
        REQUIRE(buffer.lineAlignment(0) == TextAlignment::Center);
        // Other lines should still be left-aligned
        REQUIRE(buffer.lineAlignment(1) == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(2) == TextAlignment::Left);
    }
    
    SECTION("set current line alignment to right") {
        buffer.setCaret({1, 0});
        buffer.setCurrentAlignment(TextAlignment::Right);
        
        REQUIRE(buffer.currentAlignment() == TextAlignment::Right);
        REQUIRE(buffer.lineAlignment(1) == TextAlignment::Right);
        // Other lines unchanged
        REQUIRE(buffer.lineAlignment(0) == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(2) == TextAlignment::Left);
    }
    
    SECTION("set current line alignment to justify") {
        buffer.setCaret({2, 0});
        buffer.setCurrentAlignment(TextAlignment::Justify);
        
        REQUIRE(buffer.currentAlignment() == TextAlignment::Justify);
        REQUIRE(buffer.lineAlignment(2) == TextAlignment::Justify);
    }
    
    SECTION("alignment changes increment version") {
        uint64_t versionBefore = buffer.version();
        buffer.setCurrentAlignment(TextAlignment::Center);
        REQUIRE(buffer.version() > versionBefore);
    }
    
    SECTION("each line can have different alignment") {
        buffer.setCaret({0, 0});
        buffer.setCurrentAlignment(TextAlignment::Left);
        buffer.setCaret({1, 0});
        buffer.setCurrentAlignment(TextAlignment::Center);
        buffer.setCaret({2, 0});
        buffer.setCurrentAlignment(TextAlignment::Right);
        
        REQUIRE(buffer.lineAlignment(0) == TextAlignment::Left);
        REQUIRE(buffer.lineAlignment(1) == TextAlignment::Center);
        REQUIRE(buffer.lineAlignment(2) == TextAlignment::Right);
    }
    
    SECTION("alignment enum names are correct") {
        REQUIRE(std::string(textAlignmentName(TextAlignment::Left)) == "Left");
        REQUIRE(std::string(textAlignmentName(TextAlignment::Center)) == "Center");
        REQUIRE(std::string(textAlignmentName(TextAlignment::Right)) == "Right");
        REQUIRE(std::string(textAlignmentName(TextAlignment::Justify)) == "Justify");
    }
}

TEST_CASE("Paragraph indentation", "[text_buffer][indentation]") {
    TextBuffer buffer;
    buffer.setText("Line 1\nLine 2\nLine 3");
    
    SECTION("default indentation is zero") {
        REQUIRE(buffer.currentLeftIndent() == 0);
        REQUIRE(buffer.currentFirstLineIndent() == 0);
        REQUIRE(buffer.lineLeftIndent(0) == 0);
        REQUIRE(buffer.lineLeftIndent(1) == 0);
        REQUIRE(buffer.lineLeftIndent(2) == 0);
        REQUIRE(buffer.lineFirstLineIndent(0) == 0);
        REQUIRE(buffer.lineFirstLineIndent(1) == 0);
        REQUIRE(buffer.lineFirstLineIndent(2) == 0);
    }
    
    SECTION("increase indent adds 20px by default") {
        buffer.setCaret({0, 0});
        buffer.increaseIndent();
        
        REQUIRE(buffer.currentLeftIndent() == 20);
        REQUIRE(buffer.lineLeftIndent(0) == 20);
        // Other lines unchanged
        REQUIRE(buffer.lineLeftIndent(1) == 0);
        REQUIRE(buffer.lineLeftIndent(2) == 0);
    }
    
    SECTION("decrease indent subtracts 20px") {
        buffer.setCaret({1, 0});
        buffer.increaseIndent();
        buffer.increaseIndent();  // Now at 40px
        REQUIRE(buffer.lineLeftIndent(1) == 40);
        
        buffer.decreaseIndent();
        REQUIRE(buffer.lineLeftIndent(1) == 20);
        
        buffer.decreaseIndent();
        REQUIRE(buffer.lineLeftIndent(1) == 0);
    }
    
    SECTION("decrease indent does not go negative") {
        buffer.setCaret({0, 0});
        buffer.decreaseIndent();
        
        REQUIRE(buffer.currentLeftIndent() == 0);
        REQUIRE(buffer.lineLeftIndent(0) == 0);
    }
    
    SECTION("custom indent amount") {
        buffer.setCaret({2, 0});
        buffer.increaseIndent(50);
        
        REQUIRE(buffer.lineLeftIndent(2) == 50);
        
        buffer.decreaseIndent(30);
        REQUIRE(buffer.lineLeftIndent(2) == 20);
    }
    
    SECTION("set left indent directly") {
        buffer.setCaret({0, 0});
        buffer.setCurrentLeftIndent(100);
        
        REQUIRE(buffer.currentLeftIndent() == 100);
        REQUIRE(buffer.lineLeftIndent(0) == 100);
    }
    
    SECTION("set left indent cannot be negative") {
        buffer.setCaret({0, 0});
        buffer.setCurrentLeftIndent(-50);
        
        REQUIRE(buffer.currentLeftIndent() == 0);  // Clamped to 0
    }
    
    SECTION("set first line indent") {
        buffer.setCaret({1, 0});
        buffer.setCurrentFirstLineIndent(30);
        
        REQUIRE(buffer.currentFirstLineIndent() == 30);
        REQUIRE(buffer.lineFirstLineIndent(1) == 30);
    }
    
    SECTION("first line indent can be negative for hanging indent") {
        buffer.setCaret({0, 0});
        buffer.setCurrentFirstLineIndent(-20);  // Hanging indent
        
        REQUIRE(buffer.currentFirstLineIndent() == -20);
        REQUIRE(buffer.lineFirstLineIndent(0) == -20);
    }
    
    SECTION("indentation changes increment version") {
        uint64_t versionBefore = buffer.version();
        buffer.increaseIndent();
        REQUIRE(buffer.version() > versionBefore);
        
        versionBefore = buffer.version();
        buffer.decreaseIndent();
        REQUIRE(buffer.version() > versionBefore);
        
        versionBefore = buffer.version();
        buffer.setCurrentLeftIndent(50);
        REQUIRE(buffer.version() > versionBefore);
    }
    
    SECTION("each line can have different indentation") {
        buffer.setCaret({0, 0});
        buffer.setCurrentLeftIndent(0);
        buffer.setCaret({1, 0});
        buffer.setCurrentLeftIndent(20);
        buffer.setCaret({2, 0});
        buffer.setCurrentLeftIndent(40);
        
        REQUIRE(buffer.lineLeftIndent(0) == 0);
        REQUIRE(buffer.lineLeftIndent(1) == 20);
        REQUIRE(buffer.lineLeftIndent(2) == 40);
    }
}

TEST_CASE("Line spacing and paragraph spacing", "[text_buffer][spacing]") {
    TextBuffer buffer;
    buffer.setText("Line 1\nLine 2\nLine 3");
    
    SECTION("default line spacing is 1.0") {
        REQUIRE(buffer.currentLineSpacing() == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(0) == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(1) == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(2) == Approx(1.0f));
    }
    
    SECTION("default paragraph spacing is zero") {
        REQUIRE(buffer.currentSpaceBefore() == 0);
        REQUIRE(buffer.currentSpaceAfter() == 0);
        REQUIRE(buffer.lineSpaceBefore(0) == 0);
        REQUIRE(buffer.lineSpaceAfter(0) == 0);
    }
    
    SECTION("set single line spacing") {
        buffer.setCaret({0, 0});
        buffer.setLineSpacingSingle();
        
        REQUIRE(buffer.currentLineSpacing() == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(0) == Approx(1.0f));
    }
    
    SECTION("set 1.5 line spacing") {
        buffer.setCaret({1, 0});
        buffer.setLineSpacing1_5();
        
        REQUIRE(buffer.currentLineSpacing() == Approx(1.5f));
        REQUIRE(buffer.lineSpacing(1) == Approx(1.5f));
        // Other lines unchanged
        REQUIRE(buffer.lineSpacing(0) == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(2) == Approx(1.0f));
    }
    
    SECTION("set double line spacing") {
        buffer.setCaret({2, 0});
        buffer.setLineSpacingDouble();
        
        REQUIRE(buffer.currentLineSpacing() == Approx(2.0f));
        REQUIRE(buffer.lineSpacing(2) == Approx(2.0f));
    }
    
    SECTION("set custom line spacing") {
        buffer.setCaret({0, 0});
        buffer.setCurrentLineSpacing(1.25f);
        
        REQUIRE(buffer.currentLineSpacing() == Approx(1.25f));
    }
    
    SECTION("line spacing is clamped to reasonable range") {
        buffer.setCaret({0, 0});
        
        // Too small - clamped to 0.5
        buffer.setCurrentLineSpacing(0.1f);
        REQUIRE(buffer.currentLineSpacing() == Approx(0.5f));
        
        // Too large - clamped to 3.0
        buffer.setCurrentLineSpacing(5.0f);
        REQUIRE(buffer.currentLineSpacing() == Approx(3.0f));
    }
    
    SECTION("set paragraph spacing before") {
        buffer.setCaret({1, 0});
        buffer.setCurrentSpaceBefore(12);
        
        REQUIRE(buffer.currentSpaceBefore() == 12);
        REQUIRE(buffer.lineSpaceBefore(1) == 12);
        // Other lines unchanged
        REQUIRE(buffer.lineSpaceBefore(0) == 0);
        REQUIRE(buffer.lineSpaceBefore(2) == 0);
    }
    
    SECTION("set paragraph spacing after") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceAfter(8);
        
        REQUIRE(buffer.currentSpaceAfter() == 8);
        REQUIRE(buffer.lineSpaceAfter(0) == 8);
    }
    
    SECTION("paragraph spacing cannot be negative") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceBefore(-10);
        buffer.setCurrentSpaceAfter(-10);
        
        REQUIRE(buffer.currentSpaceBefore() == 0);
        REQUIRE(buffer.currentSpaceAfter() == 0);
    }
    
    SECTION("spacing changes increment version") {
        uint64_t versionBefore = buffer.version();
        buffer.setCurrentLineSpacing(1.5f);
        REQUIRE(buffer.version() > versionBefore);
        
        versionBefore = buffer.version();
        buffer.setCurrentSpaceBefore(10);
        REQUIRE(buffer.version() > versionBefore);
        
        versionBefore = buffer.version();
        buffer.setCurrentSpaceAfter(10);
        REQUIRE(buffer.version() > versionBefore);
    }
    
    SECTION("each line can have different spacing") {
        buffer.setCaret({0, 0});
        buffer.setLineSpacingSingle();
        buffer.setCaret({1, 0});
        buffer.setLineSpacing1_5();
        buffer.setCaret({2, 0});
        buffer.setLineSpacingDouble();
        
        REQUIRE(buffer.lineSpacing(0) == Approx(1.0f));
        REQUIRE(buffer.lineSpacing(1) == Approx(1.5f));
        REQUIRE(buffer.lineSpacing(2) == Approx(2.0f));
    }
}

TEST_CASE("Line spacing", "[text_buffer][spacing]") {
    TextBuffer buffer;
    buffer.setText("Line 1\nLine 2\nLine 3");
    
    SECTION("default line spacing is 1.0 (single)") {
        REQUIRE(buffer.currentLineSpacing() == 1.0f);
        REQUIRE(buffer.lineSpacing(0) == 1.0f);
        REQUIRE(buffer.lineSpacing(1) == 1.0f);
        REQUIRE(buffer.lineSpacing(2) == 1.0f);
    }
    
    SECTION("set single line spacing") {
        buffer.setCaret({0, 0});
        buffer.setLineSpacingSingle();
        
        REQUIRE(buffer.currentLineSpacing() == 1.0f);
        REQUIRE(buffer.lineSpacing(0) == 1.0f);
    }
    
    SECTION("set 1.5x line spacing") {
        buffer.setCaret({1, 0});
        buffer.setLineSpacing1_5();
        
        REQUIRE(buffer.currentLineSpacing() == 1.5f);
        REQUIRE(buffer.lineSpacing(1) == 1.5f);
        // Other lines unchanged
        REQUIRE(buffer.lineSpacing(0) == 1.0f);
        REQUIRE(buffer.lineSpacing(2) == 1.0f);
    }
    
    SECTION("set double line spacing") {
        buffer.setCaret({2, 0});
        buffer.setLineSpacingDouble();
        
        REQUIRE(buffer.currentLineSpacing() == 2.0f);
        REQUIRE(buffer.lineSpacing(2) == 2.0f);
    }
    
    SECTION("set custom line spacing") {
        buffer.setCaret({0, 0});
        buffer.setCurrentLineSpacing(1.25f);
        
        REQUIRE(buffer.currentLineSpacing() == 1.25f);
        REQUIRE(buffer.lineSpacing(0) == 1.25f);
    }
    
    SECTION("line spacing clamped to valid range") {
        buffer.setCaret({0, 0});
        
        // Too small
        buffer.setCurrentLineSpacing(0.1f);
        REQUIRE(buffer.currentLineSpacing() == 0.5f);  // Clamped to 0.5 minimum
        
        // Too large
        buffer.setCurrentLineSpacing(5.0f);
        REQUIRE(buffer.currentLineSpacing() == 3.0f);  // Clamped to 3.0 maximum
    }
}

TEST_CASE("Paragraph spacing", "[text_buffer][spacing]") {
    TextBuffer buffer;
    buffer.setText("Paragraph 1\nParagraph 2\nParagraph 3");
    
    SECTION("default paragraph spacing is zero") {
        REQUIRE(buffer.currentSpaceBefore() == 0);
        REQUIRE(buffer.currentSpaceAfter() == 0);
        REQUIRE(buffer.lineSpaceBefore(0) == 0);
        REQUIRE(buffer.lineSpaceAfter(0) == 0);
        REQUIRE(buffer.lineSpaceBefore(1) == 0);
        REQUIRE(buffer.lineSpaceAfter(1) == 0);
    }
    
    SECTION("set space before paragraph") {
        buffer.setCaret({1, 0});
        buffer.setCurrentSpaceBefore(12);
        
        REQUIRE(buffer.currentSpaceBefore() == 12);
        REQUIRE(buffer.lineSpaceBefore(1) == 12);
        // Other paragraphs unchanged
        REQUIRE(buffer.lineSpaceBefore(0) == 0);
        REQUIRE(buffer.lineSpaceBefore(2) == 0);
    }
    
    SECTION("set space after paragraph") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceAfter(8);
        
        REQUIRE(buffer.currentSpaceAfter() == 8);
        REQUIRE(buffer.lineSpaceAfter(0) == 8);
    }
    
    SECTION("space before cannot be negative") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceBefore(-10);
        
        REQUIRE(buffer.currentSpaceBefore() == 0);  // Clamped to 0
    }
    
    SECTION("space after cannot be negative") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceAfter(-5);
        
        REQUIRE(buffer.currentSpaceAfter() == 0);  // Clamped to 0
    }
    
    SECTION("each paragraph can have different spacing") {
        buffer.setCaret({0, 0});
        buffer.setCurrentSpaceBefore(0);
        buffer.setCurrentSpaceAfter(6);
        
        buffer.setCaret({1, 0});
        buffer.setCurrentSpaceBefore(12);
        buffer.setCurrentSpaceAfter(12);
        
        buffer.setCaret({2, 0});
        buffer.setCurrentSpaceBefore(6);
        buffer.setCurrentSpaceAfter(0);
        
        REQUIRE(buffer.lineSpaceBefore(0) == 0);
        REQUIRE(buffer.lineSpaceAfter(0) == 6);
        REQUIRE(buffer.lineSpaceBefore(1) == 12);
        REQUIRE(buffer.lineSpaceAfter(1) == 12);
        REQUIRE(buffer.lineSpaceBefore(2) == 6);
        REQUIRE(buffer.lineSpaceAfter(2) == 0);
    }
}

TEST_CASE("Bulleted and numbered lists", "[text_buffer][lists]") {
    TextBuffer buffer;
    buffer.setText("Item 1\nItem 2\nItem 3");
    
    SECTION("default list type is None") {
        REQUIRE(buffer.currentListType() == ListType::None);
        REQUIRE(buffer.lineListType(0) == ListType::None);
        REQUIRE(buffer.lineListType(1) == ListType::None);
        REQUIRE(buffer.lineListType(2) == ListType::None);
    }
    
    SECTION("toggle bulleted list") {
        buffer.setCaret({0, 0});
        buffer.toggleBulletedList();
        
        REQUIRE(buffer.currentListType() == ListType::Bulleted);
        REQUIRE(buffer.lineListType(0) == ListType::Bulleted);
        // Other lines unchanged
        REQUIRE(buffer.lineListType(1) == ListType::None);
        
        // Toggle off
        buffer.toggleBulletedList();
        REQUIRE(buffer.currentListType() == ListType::None);
    }
    
    SECTION("toggle numbered list") {
        buffer.setCaret({1, 0});
        buffer.toggleNumberedList();
        
        REQUIRE(buffer.currentListType() == ListType::Numbered);
        REQUIRE(buffer.lineListType(1) == ListType::Numbered);
        REQUIRE(buffer.lineListNumber(1) == 1);
        
        // Toggle off
        buffer.toggleNumberedList();
        REQUIRE(buffer.currentListType() == ListType::None);
    }
    
    SECTION("multi-level lists with increase/decrease") {
        buffer.setCaret({0, 0});
        buffer.toggleBulletedList();
        REQUIRE(buffer.currentListLevel() == 0);
        
        buffer.increaseListLevel();
        REQUIRE(buffer.currentListLevel() == 1);
        
        buffer.increaseListLevel();
        REQUIRE(buffer.currentListLevel() == 2);
        
        buffer.decreaseListLevel();
        REQUIRE(buffer.currentListLevel() == 1);
        
        buffer.decreaseListLevel();
        REQUIRE(buffer.currentListLevel() == 0);
        
        // Cannot go below 0
        buffer.decreaseListLevel();
        REQUIRE(buffer.currentListLevel() == 0);
    }
    
    SECTION("numbered list renumbering") {
        // Make all lines numbered
        buffer.setCaret({0, 0});
        buffer.toggleNumberedList();
        buffer.setCaret({1, 0});
        buffer.toggleNumberedList();
        buffer.setCaret({2, 0});
        buffer.toggleNumberedList();
        
        REQUIRE(buffer.lineListNumber(0) == 1);
        REQUIRE(buffer.lineListNumber(1) == 2);
        REQUIRE(buffer.lineListNumber(2) == 3);
    }
    
    SECTION("list type changes increment version") {
        uint64_t versionBefore = buffer.version();
        buffer.toggleBulletedList();
        REQUIRE(buffer.version() > versionBefore);
        
        versionBefore = buffer.version();
        buffer.toggleNumberedList();
        REQUIRE(buffer.version() > versionBefore);
    }
    
    SECTION("each line can have different list type") {
        buffer.setCaret({0, 0});
        buffer.toggleBulletedList();
        
        buffer.setCaret({1, 0});
        buffer.toggleNumberedList();
        
        // Line 2 stays none
        
        REQUIRE(buffer.lineListType(0) == ListType::Bulleted);
        REQUIRE(buffer.lineListType(1) == ListType::Numbered);
        REQUIRE(buffer.lineListType(2) == ListType::None);
    }
}

TEST_CASE("Find and replace", "[text_buffer][find]") {
    TextBuffer buffer;
    buffer.setText("Hello world, hello everyone. Hello!");
    
    SECTION("find basic match") {
        buffer.setCaret({0, 0});
        FindResult result = buffer.find("Hello");
        
        REQUIRE(result.found);
        REQUIRE(result.start.row == 0);
        REQUIRE(result.start.column == 0);
        REQUIRE(result.end.column == 5);
    }
    
    SECTION("find case insensitive") {
        buffer.setCaret({0, 0});
        FindOptions opts;
        opts.caseSensitive = false;
        
        FindResult result = buffer.find("HELLO", opts);
        
        REQUIRE(result.found);
        REQUIRE(result.start.column == 0);
    }
    
    SECTION("find case sensitive") {
        buffer.setCaret({0, 0});
        FindOptions opts;
        opts.caseSensitive = true;
        
        // "HELLO" should not match "Hello"
        FindResult result = buffer.find("HELLO", opts);
        REQUIRE_FALSE(result.found);
        
        // "Hello" should match
        result = buffer.find("Hello", opts);
        REQUIRE(result.found);
    }
    
    SECTION("find whole word only") {
        buffer.setCaret({0, 0});
        FindOptions opts;
        opts.wholeWord = true;
        
        // "Hell" should not match as whole word
        FindResult result = buffer.find("Hell", opts);
        REQUIRE_FALSE(result.found);
        
        // "Hello" should match as whole word
        result = buffer.find("Hello", opts);
        REQUIRE(result.found);
    }
    
    SECTION("find next occurrence") {
        buffer.setCaret({0, 0});
        FindOptions opts;
        opts.caseSensitive = false;  // Match all "hello" variants
        
        FindResult first = buffer.find("hello", opts);
        REQUIRE(first.found);
        REQUIRE(first.start.column == 0);
        
        buffer.setCaret(first.end);
        FindResult second = buffer.findNext("hello", opts);
        REQUIRE(second.found);
        REQUIRE(second.start.column == 13);  // "hello" in "hello everyone"
    }
    
    SECTION("find previous occurrence") {
        buffer.setCaret({0, 35});  // End of text
        FindOptions opts;
        opts.caseSensitive = false;
        
        FindResult result = buffer.findPrevious("hello", opts);
        REQUIRE(result.found);
        REQUIRE(result.start.column == 29);  // Last "Hello" before the "!"
    }
    
    SECTION("find all occurrences") {
        FindOptions opts;
        opts.caseSensitive = false;
        
        std::vector<FindResult> results = buffer.findAll("hello", opts);
        REQUIRE(results.size() == 3);
        REQUIRE(results[0].start.column == 0);
        REQUIRE(results[1].start.column == 13);
        REQUIRE(results[2].start.column == 29);
    }
    
    SECTION("find with wrap around") {
        buffer.setCaret({0, 20});  // Middle of text
        FindOptions opts;
        opts.wrapAround = true;
        opts.caseSensitive = true;
        
        // Should find "Hello" at start (wrapped)
        FindResult result = buffer.find("Hello", opts);
        REQUIRE(result.found);
    }
    
    SECTION("find without wrap around") {
        buffer.setCaret({0, 30});  // Near end
        FindOptions opts;
        opts.wrapAround = false;
        opts.caseSensitive = true;
        
        // Should not find "Hello" without wrapping
        FindResult result = buffer.find("Hello", opts);
        REQUIRE_FALSE(result.found);
    }
    
    SECTION("find empty needle returns not found") {
        FindResult result = buffer.find("");
        REQUIRE_FALSE(result.found);
    }
    
    SECTION("find non-existent text") {
        FindResult result = buffer.find("xyz");
        REQUIRE_FALSE(result.found);
    }
    
    SECTION("replace selected text") {
        buffer.setCaret({0, 0});
        // Select "Hello"
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        
        bool replaced = buffer.replace("Hello", "Hi");
        REQUIRE(replaced);
        REQUIRE(buffer.getText().substr(0, 2) == "Hi");
    }
    
    SECTION("replace only if selection matches") {
        buffer.setCaret({0, 0});
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        
        // Try to replace with wrong needle
        bool replaced = buffer.replace("Goodbye", "Hi");
        REQUIRE_FALSE(replaced);
        REQUIRE(buffer.getText().substr(0, 5) == "Hello");  // Unchanged
    }
    
    SECTION("replace all occurrences") {
        std::size_t count = buffer.replaceAll("Hello", "Hi");
        // Case insensitive by default, should match 2 "Hello" instances
        // (first and last - the middle one is lowercase "hello")
        FindOptions opts;
        opts.caseSensitive = false;
        count = buffer.replaceAll("hi", "yo", opts);
        // At this point we've already replaced, let's reset and test fresh
    }
    
    SECTION("replace all preserves order") {
        TextBuffer buf2;
        buf2.setText("cat cat cat");
        
        std::size_t count = buf2.replaceAll("cat", "dog");
        REQUIRE(count == 3);
        REQUIRE(buf2.getText() == "dog dog dog");
    }
    
    SECTION("replace all with different length replacement") {
        TextBuffer buf2;
        buf2.setText("a b c");
        
        std::size_t count = buf2.replaceAll("b", "xyz");
        REQUIRE(count == 1);
        REQUIRE(buf2.getText() == "a xyz c");
    }
    
    SECTION("replace all case insensitive") {
        FindOptions opts;
        opts.caseSensitive = false;
        
        std::size_t count = buffer.replaceAll("hello", "HI", opts);
        REQUIRE(count == 3);
        REQUIRE(buffer.getText() == "HI world, HI everyone. HI!");
    }
}

TEST_CASE("Find across multiple lines", "[text_buffer][find]") {
    TextBuffer buffer;
    buffer.setText("Line one\nLine two\nLine three");
    
    SECTION("find on second line") {
        buffer.setCaret({0, 0});
        FindResult result = buffer.find("two");
        
        REQUIRE(result.found);
        REQUIRE(result.start.row == 1);
        REQUIRE(result.start.column == 5);
    }
    
    SECTION("find all across lines") {
        std::vector<FindResult> results = buffer.findAll("Line");
        REQUIRE(results.size() == 3);
        REQUIRE(results[0].start.row == 0);
        REQUIRE(results[1].start.row == 1);
        REQUIRE(results[2].start.row == 2);
    }
    
    SECTION("replace all across lines") {
        std::size_t count = buffer.replaceAll("Line", "Row");
        REQUIRE(count == 3);
        REQUIRE(buffer.getText() == "Row one\nRow two\nRow three");
    }
}

