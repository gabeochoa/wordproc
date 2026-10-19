#include "../src/editor/format_painter.h"

#include "catch2/catch.hpp"

// FormatPainter copy/apply logic at buffer level. Per-range
// (Batch 12): capture reads the effective style at the
// selection/caret, apply writes a per-range span, so painting
// one word leaves its neighbours alone. The no-range apply()
// still exists for callers with no target (bare caret on
// whitespace sets the global typing style).

TEST_CASE("FormatPainter - copy/apply/disarm", "[format-painter]") {
    SECTION("apply with nothing armed is a no-op") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        REQUIRE_FALSE(painter.armed);
        REQUIRE_FALSE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.textStyle().bold);
    }

    SECTION("arm captures the style; apply to a range sets it and disarms") {
        TextBuffer buffer;
        buffer.setText("hello world");
        TextStyle bold;
        bold.bold = true;
        bold.italic = true;
        bold.fontSize = 20;
        bold.font = "Arial";
        FormatPainter painter;
        painter.arm(bold);
        REQUIRE(painter.armed);
        REQUIRE(painter.captured.bold);
        REQUIRE(painter.apply(buffer, 0, 5));
        REQUIRE_FALSE(painter.armed);
        REQUIRE(buffer.effectiveStyleAt(0).bold);
        REQUIRE(buffer.effectiveStyleAt(0).italic);
        REQUIRE(buffer.effectiveStyleAt(0).fontSize == 20);
        REQUIRE(buffer.effectiveStyleAt(0).font == "Arial");
        // Outside the painted range the global (plain) style holds.
        REQUIRE_FALSE(buffer.effectiveStyleAt(6).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(6).italic);
    }

    SECTION("apply disarms, so a second apply is a no-op") {
        TextBuffer buffer;
        buffer.setText("hello");
        FormatPainter painter;
        TextStyle style;
        style.underline = true;
        painter.arm(style);
        REQUIRE(painter.apply(buffer, 0, 5));
        REQUIRE_FALSE(painter.apply(buffer, 0, 5));
        // Reset the span to prove the second apply changes nothing.
        TextStyle plain;
        buffer.applyCharStyleRange(0, 5, plain);
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).underline);
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
        REQUIRE_FALSE(painter.apply(buffer, 0, 5));
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
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
        REQUIRE(painter.apply(buffer, 0, 5));
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
        REQUIRE(buffer.effectiveStyleAt(0).italic);
    }
}

TEST_CASE("FormatPainter - per-range paint leaves neighbours alone",
          "[format-painter]") {
    SECTION("bold word A, paint onto word B: B bold, A unchanged, C plain") {
        TextBuffer buffer;
        buffer.setText("alpha beta gamma");
        // Bold word A ("alpha", [0, 5)) through the same span
        // store the Bold button writes.
        TextStyle bold;
        bold.bold = true;
        buffer.applyCharStyleRange(0, 5, bold);
        REQUIRE(buffer.effectiveStyleAt(0).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(6).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(11).bold);

        // Capture at A, paint onto word B ("beta", [6, 10)).
        FormatPainter painter;
        painter.arm(buffer.effectiveStyleAt(0));
        REQUIRE(painter.apply(buffer, 6, 10));

        REQUIRE(buffer.effectiveStyleAt(0).bold);   // A unchanged
        REQUIRE(buffer.effectiveStyleAt(7).bold);   // B bold
        REQUIRE_FALSE(buffer.effectiveStyleAt(12).bold);  // C plain
        REQUIRE_FALSE(buffer.effectiveStyleAt(5).bold);   // space plain
        REQUIRE(buffer.charStyleSpans().size() == 2);
    }

    SECTION("apply() with no range paints the selection") {
        TextBuffer buffer;
        buffer.setText("alpha beta gamma");
        buffer.selectWordAt({0, 7});  // selects "beta"
        REQUIRE(buffer.getSelectedText() == "beta");
        FormatPainter painter;
        TextStyle bold;
        bold.bold = true;
        painter.arm(bold);
        REQUIRE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
        REQUIRE(buffer.effectiveStyleAt(7).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(12).bold);
    }

    SECTION("apply() with no selection paints the word at the caret") {
        TextBuffer buffer;
        buffer.setText("alpha beta gamma");
        buffer.setCaret({0, 7});  // inside "beta"
        FormatPainter painter;
        TextStyle bold;
        bold.bold = true;
        painter.arm(bold);
        REQUIRE(painter.apply(buffer));
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
        REQUIRE(buffer.effectiveStyleAt(7).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(12).bold);
    }

    SECTION("caret at a span's end reads the span (insertion-point rule)") {
        TextBuffer buffer;
        buffer.setText("alpha beta");
        TextStyle bold;
        bold.bold = true;
        buffer.applyCharStyleRange(0, 5, bold);
        buffer.setCaret({0, 5});  // just past "alpha"
        REQUIRE(buffer.effectiveStyleAtCaretOrSelection().bold);
        buffer.setCaret({0, 6});  // on "beta" (space at 5, then 'b')
        REQUIRE_FALSE(buffer.effectiveStyleAtCaretOrSelection().bold);
        // Toggling Bold at the span end unbolds the whole span.
        buffer.setCaret({0, 5});
        TextStyle plain = buffer.effectiveStyleAtCaretOrSelection();
        plain.bold = false;
        buffer.applyStyleToCurrentRange(plain);
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
    }

    SECTION("capture reads the effective style, not the global style") {
        TextBuffer buffer;
        buffer.setText("alpha beta");
        TextStyle bold;
        bold.bold = true;
        buffer.applyCharStyleRange(0, 5, bold);
        // Global is plain; caret inside the bold span captures bold.
        REQUIRE_FALSE(buffer.textStyle().bold);
        buffer.setCaret({0, 2});
        REQUIRE(buffer.effectiveStyleAtCaretOrSelection().bold);
    }
}

TEST_CASE("TextBuffer - char style spans shift with edits",
          "[format-painter]") {
    SECTION("typing inside a span expands it; typing before it shifts it") {
        TextBuffer buffer;
        buffer.setText("alpha beta");
        TextStyle bold;
        bold.bold = true;
        buffer.applyCharStyleRange(0, 5, bold);
        buffer.setCaret({0, 2});
        buffer.insertChar('X');  // "alXpha beta", span [0, 6)
        REQUIRE(buffer.getText() == "alXpha beta");
        REQUIRE(buffer.effectiveStyleAt(2).bold);
        REQUIRE(buffer.effectiveStyleAt(5).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(6).bold);
        buffer.setCaret({0, 0});
        buffer.insertChar('Y');  // "YalXpha beta", span [1, 7)
        REQUIRE_FALSE(buffer.effectiveStyleAt(0).bold);
        REQUIRE(buffer.effectiveStyleAt(1).bold);
    }

    SECTION("deleting inside a span clips it; deleting after it is neutral") {
        TextBuffer buffer;
        buffer.setText("alpha beta");
        TextStyle bold;
        bold.bold = true;
        buffer.applyCharStyleRange(0, 5, bold);
        buffer.setCaret({0, 2});
        buffer.del();  // delete 'p' -> "alha beta", span [0, 4)
        REQUIRE(buffer.getText() == "alha beta");
        REQUIRE(buffer.effectiveStyleAt(3).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(4).bold);
        REQUIRE(buffer.charStyleSpans().size() == 1);
        REQUIRE(buffer.charStyleSpans()[0].endOffset == 4);
    }

    SECTION("Bold through applyStyleToCurrentRange bolds only the selection") {
        TextBuffer buffer;
        buffer.setText("alpha beta gamma");
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        TextStyle bold;
        bold.bold = true;
        buffer.applyStyleToCurrentRange(bold);
        REQUIRE(buffer.effectiveStyleAt(0).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(6).bold);
        // The global typing style is untouched by a selection apply.
        REQUIRE_FALSE(buffer.textStyle().bold);
    }

    SECTION("overlapping applies clip the old span") {
        TextBuffer buffer;
        buffer.setText("abcdefgh");
        TextStyle bold;
        bold.bold = true;
        TextStyle italic;
        italic.italic = true;
        buffer.applyCharStyleRange(0, 6, bold);
        buffer.applyCharStyleRange(3, 8, italic);
        REQUIRE(buffer.effectiveStyleAt(1).bold);
        REQUIRE_FALSE(buffer.effectiveStyleAt(1).italic);
        REQUIRE_FALSE(buffer.effectiveStyleAt(4).bold);
        REQUIRE(buffer.effectiveStyleAt(4).italic);
        REQUIRE(buffer.charStyleSpans().size() == 2);
    }
}
