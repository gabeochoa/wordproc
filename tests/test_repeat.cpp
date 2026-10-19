#include "../src/editor/repeat.h"
#include "../src/editor/text_buffer.h"

#include "catch2/catch.hpp"

TEST_CASE("Repeat noteTyped - typing coalesces into one run",
          "[repeat]") {
    ActionRecord record;

    SECTION("Consecutive typed units coalesce") {
        repeat::noteTyped(record, "a", 0, false);
        repeat::noteTyped(record, "b", 1, false);
        REQUIRE(record.kind == RepeatKind::InsertText);
        REQUIRE(record.text == "ab");
        REQUIRE(record.typedEndOffset == 2);
    }
    SECTION("Moving the caret starts a new run") {
        repeat::noteTyped(record, "a", 0, false);
        repeat::noteTyped(record, "b", 1, false);
        repeat::noteTyped(record, "c", 0, false);  // moved to start
        REQUIRE(record.text == "c");
    }
    SECTION("Typing over a selection starts a new run") {
        repeat::noteTyped(record, "ab", 0, false);
        repeat::noteTyped(record, "x", 2, true);
        REQUIRE(record.text == "x");
    }
    SECTION("Empty typed text is ignored") {
        repeat::noteTyped(record, "", 0, false);
        REQUIRE(record.kind == RepeatKind::None);
    }
    SECTION("clear empties the record") {
        repeat::noteTyped(record, "ab", 0, false);
        REQUIRE(repeat::hasAction(record));
        repeat::clear(record);
        REQUIRE_FALSE(repeat::hasAction(record));
        REQUIRE(record.kind == RepeatKind::None);
    }
}

TEST_CASE("Repeat apply - InsertText at the caret, replacing a selection",
          "[repeat]") {
    TextBuffer buffer;
    buffer.setText("hello");
    buffer.setCaret({0, 5});

    SECTION("Inserts at the caret") {
        repeat::apply(buffer, repeat::setText(RepeatKind::InsertText, "ab"));
        REQUIRE(buffer.getText() == "helloab");
    }
    SECTION("Replaces a selection") {
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        REQUIRE(buffer.hasSelection());
        repeat::apply(buffer, repeat::setText(RepeatKind::InsertText, "ab"));
        REQUIRE(buffer.getText() == "ab");
    }
    SECTION("Paste inserts the pasted text") {
        repeat::apply(buffer, repeat::setText(RepeatKind::Paste, "XY"));
        REQUIRE(buffer.getText() == "helloXY");
    }
    SECTION("None is a no-op") {
        repeat::apply(buffer, ActionRecord{});
        REQUIRE(buffer.getText() == "hello");
    }
}

TEST_CASE("Repeat apply - formatting re-applies the recorded value",
          "[repeat]") {
    TextBuffer buffer;
    buffer.setText("hello");

    SECTION("SetBold(true) bolds from unbolded") {
        REQUIRE_FALSE(buffer.textStyle().bold);
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetBold, true));
        REQUIRE(buffer.textStyle().bold);
        // Re-applying stays bold (idempotent, not a toggle).
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetBold, true));
        REQUIRE(buffer.textStyle().bold);
    }
    SECTION("SetBold(false) un-bolds") {
        TextStyle style = buffer.textStyle();
        style.bold = true;
        buffer.setTextStyle(style);
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetBold, false));
        REQUIRE_FALSE(buffer.textStyle().bold);
    }
    SECTION("SetItalic / SetUnderline / SetStrikethrough") {
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetItalic, true));
        REQUIRE(buffer.textStyle().italic);
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetUnderline, true));
        REQUIRE(buffer.textStyle().underline);
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetStrikethrough, true));
        REQUIRE(buffer.textStyle().strikethrough);
    }
    SECTION("SetSuperscript clears Subscript and vice versa") {
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetSubscript, true));
        REQUIRE(buffer.textStyle().subscript);
        repeat::apply(buffer,
                      repeat::setBool(RepeatKind::SetSuperscript, true));
        REQUIRE(buffer.textStyle().superscript);
        REQUIRE_FALSE(buffer.textStyle().subscript);
    }
    SECTION("SetParagraphStyle / SetFontSize / SetFont") {
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetParagraphStyle,
                                             static_cast<int>(ParagraphStyle::Heading1)));
        REQUIRE(buffer.currentParagraphStyle() == ParagraphStyle::Heading1);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetFontSize, 20));
        REQUIRE(buffer.textStyle().fontSize == 20);
        repeat::apply(buffer,
                      repeat::setText(RepeatKind::SetFont, "Arial"));
        REQUIRE(buffer.textStyle().font == "Arial");
    }
    SECTION("SetTextColor / SetHighlightColor") {
        repeat::apply(buffer,
                      repeat::setColor(RepeatKind::SetTextColor, TextColors::Red));
        REQUIRE(buffer.textStyle().textColor == TextColors::Red);
        repeat::apply(buffer,
                      repeat::setColor(RepeatKind::SetHighlightColor,
                                       HighlightColors::Yellow));
        REQUIRE(buffer.textStyle().highlightColor == HighlightColors::Yellow);
    }
    SECTION("SetAlignment / SetLeftIndent / SetSpaceBefore / SetSpaceAfter") {
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetAlignment,
                                             static_cast<int>(TextAlignment::Center)));
        REQUIRE(buffer.currentAlignment() == TextAlignment::Center);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetLeftIndent, 40));
        REQUIRE(buffer.currentLeftIndent() == 40);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetSpaceBefore, 12));
        REQUIRE(buffer.currentSpaceBefore() == 12);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetSpaceAfter, 6));
        REQUIRE(buffer.currentSpaceAfter() == 6);
    }
    SECTION("SetLineSpacing 150 -> 1.5") {
        repeat::apply(buffer,
                      repeat::setInt(RepeatKind::SetLineSpacing, 150));
        REQUIRE(buffer.currentLineSpacing() == 1.5f);
    }
    SECTION("SetListType drives the toggle API to the target") {
        REQUIRE(buffer.currentListType() == ListType::None);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetListType,
                                             static_cast<int>(ListType::Bulleted)));
        REQUIRE(buffer.currentListType() == ListType::Bulleted);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetListType,
                                             static_cast<int>(ListType::Numbered)));
        REQUIRE(buffer.currentListType() == ListType::Numbered);
        repeat::apply(buffer, repeat::setInt(RepeatKind::SetListType,
                                             static_cast<int>(ListType::None)));
        REQUIRE(buffer.currentListType() == ListType::None);
    }
}

TEST_CASE("Repeat apply - does not modify the record (F4 F4 repeats)",
          "[repeat]") {
    TextBuffer buffer;
    buffer.setText("x");
    buffer.setCaret({0, 1});
    ActionRecord record = repeat::setText(RepeatKind::InsertText, "ab");
    repeat::apply(buffer, record);
    REQUIRE(buffer.getText() == "xab");
    REQUIRE(record.text == "ab");
    REQUIRE(record.kind == RepeatKind::InsertText);
}
