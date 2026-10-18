#include "../src/editor/format_painter.h"

#include "catch2/catch.hpp"

// FormatPainter copy/apply logic at buffer level. The buffer's
// style is document-global (see format_painter.h for the subset
// note), so apply sets the buffer's TextStyle and disarms.

TEST_CASE("FormatPainter - copy/apply/disarm", "[format-painter]") {
    SECTION("apply with nothing armed is a no-op") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        REQUIRE_FALSE(painter.armed);
        REQUIRE_FALSE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.textStyle().bold);
    }

    SECTION("arm captures the style; apply sets it and disarms") {
        TextBuffer buffer;
        buffer.setText("hello");
        TextStyle bold;
        bold.bold = true;
        bold.italic = true;
        bold.fontSize = 20;
        bold.font = "Arial";
        FormatPainter painter;
        painter.arm(bold);
        REQUIRE(painter.armed);
        REQUIRE(painter.captured.bold);
        REQUIRE(painter.apply(buffer));
        REQUIRE_FALSE(painter.armed);
        TextStyle applied = buffer.textStyle();
        REQUIRE(applied.bold);
        REQUIRE(applied.italic);
        REQUIRE(applied.fontSize == 20);
        REQUIRE(applied.font == "Arial");
    }

    SECTION("apply disarms, so a second apply is a no-op") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        TextStyle style;
        style.underline = true;
        painter.arm(style);
        REQUIRE(painter.apply(buffer));
        // Reset the buffer style to prove the second apply changes
        // nothing (it must not re-apply after disarming).
        TextStyle plain;
        buffer.setTextStyle(plain);
        REQUIRE_FALSE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.textStyle().underline);
    }

    SECTION("disarm cancels without applying") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        TextStyle style;
        style.bold = true;
        painter.arm(style);
        painter.disarm();
        REQUIRE_FALSE(painter.armed);
        REQUIRE_FALSE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.textStyle().bold);
    }

    SECTION("re-arm captures the newer style") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        TextStyle first;
        first.bold = true;
        TextStyle second;
        second.italic = true;
        painter.arm(first);
        painter.arm(second);
        REQUIRE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.textStyle().bold);
        REQUIRE(buffer.textStyle().italic);
    }
}
