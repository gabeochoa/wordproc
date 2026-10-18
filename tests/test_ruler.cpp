#include "../src/editor/ruler_math.h"
#include "../src/editor/text_buffer.h"
#include "../src/ui/menu_types.h"
#include "catch2/catch.hpp"

TEST_CASE("ruler marker-x <-> indent mapping", "[ruler]") {
    // Pageless: text area at x=3, ruler above it; effective = text area.
    auto g = ruler_math::geometry(123.0f, 20.0f, 3.0f, 1274.0f, 0.0f);
    SECTION("origin sits at effective x + text padding") {
        REQUIRE(g.originX == 11.0f);
        REQUIRE(g.rulerY == 103.0f);
        REQUIRE(g.rulerHeight == 20.0f);
        REQUIRE(g.textSpanWidth == 1274.0f - 16.0f);
    }
    SECTION("marker positions follow the indent triple") {
        REQUIRE(ruler_math::leftMarkerX(g, 0) == 11.0f);
        REQUIRE(ruler_math::leftMarkerX(g, 72) == 83.0f);
        REQUIRE(ruler_math::firstLineMarkerX(g, 72, 36) == 119.0f);
        REQUIRE(ruler_math::firstLineMarkerX(g, 0, -18) == -7.0f);
        REQUIRE(ruler_math::rightMarkerX(g, 0) == 11.0f + g.textSpanWidth);
        REQUIRE(ruler_math::rightMarkerX(g, 72) ==
                11.0f + g.textSpanWidth - 72.0f);
    }
    SECTION("inverse mapping round-trips and clamps") {
        REQUIRE(ruler_math::indentFromX(g, 83.0f) == 72);
        REQUIRE(ruler_math::indentFromX(g, -100.0f) == 0);
        REQUIRE(ruler_math::indentFromX(g, 100000.0f) ==
                static_cast<int>(g.textSpanWidth));
        REQUIRE(ruler_math::firstLineIndentFromX(g, 47.0f, 0) == 36);
        // Hanging: first line may go left of the left indent, but not
        // left of the margin.
        REQUIRE(ruler_math::firstLineIndentFromX(g, -50.0f, 36) == -36);
        REQUIRE(ruler_math::rightIndentFromX(g, 11.0f + g.textSpanWidth -
                                                     72.0f) == 72);
        REQUIRE(ruler_math::rightIndentFromX(g, -100.0f) ==
                static_cast<int>(g.textSpanWidth));
    }
    SECTION("hit test splits top/bottom halves") {
        float rulerY = g.rulerY;
        REQUIRE(ruler_math::hitTest(g, 0, 0, 0, 11.0f, rulerY + 5.0f) ==
                ruler_math::Marker::FirstLine);
        REQUIRE(ruler_math::hitTest(g, 0, 0, 0, 11.0f, rulerY + 15.0f) ==
                ruler_math::Marker::Left);
        float rightX = ruler_math::rightMarkerX(g, 0);
        REQUIRE(ruler_math::hitTest(g, 0, 0, 0, rightX, rulerY + 15.0f) ==
                ruler_math::Marker::Right);
        REQUIRE(ruler_math::hitTest(g, 0, 0, 0, 500.0f, rulerY + 15.0f) ==
                ruler_math::Marker::None);
        REQUIRE(ruler_math::hitTest(g, 0, 0, 0, 11.0f, rulerY - 5.0f) ==
                ruler_math::Marker::None);
    }
    SECTION("tab stops are one per tabWidth spaces") {
        auto stops = ruler_math::tabStopXs(g, 4, 8.0f);
        REQUIRE(!stops.empty());
        REQUIRE(stops[0] == 11.0f + 32.0f);
        REQUIRE(stops[1] == 11.0f + 64.0f);
        REQUIRE(ruler_math::tabStopXs(g, 0, 8.0f).empty());
    }
    SECTION("paged geometry anchors on the effective (margin) area") {
        // Page at x=400, margins leave effective x=450, width=300.
        auto paged = ruler_math::geometry(123.0f, 20.0f, 450.0f, 300.0f, 0.0f);
        REQUIRE(paged.originX == 458.0f);
        REQUIRE(paged.whiteLeft == 450.0f);
        REQUIRE(paged.whiteRight == 750.0f);
        REQUIRE(ruler_math::leftMarkerX(paged, 72) == 530.0f);
    }
}

TEST_CASE("paragraph indents are one undo step", "[ruler]") {
    TextBuffer buffer;
    buffer.setText("hello");
    SECTION("right indent defaults to zero and applies") {
        REQUIRE(buffer.currentRightIndent() == 0);
        buffer.setCurrentRightIndent(48);
        REQUIRE(buffer.currentRightIndent() == 48);
        REQUIRE(buffer.lineIndents(0).right == 48);
        buffer.setCurrentRightIndent(-5);
        REQUIRE(buffer.currentRightIndent() == 0);
    }
    SECTION("drag commit is a single undo step") {
        ParagraphIndents oldIndents = buffer.lineIndents(0);
        buffer.setLineIndents(0, {72, 36, 48});
        REQUIRE(buffer.currentLeftIndent() == 72);
        buffer.commitParagraphIndents(0, oldIndents, {72, 36, 48});
        REQUIRE(buffer.canUndo());
        buffer.undo();
        REQUIRE(buffer.lineIndents(0) == oldIndents);
        buffer.redo();
        REQUIRE(buffer.lineIndents(0) == ParagraphIndents{72, 36, 48});
    }
    SECTION("commit with no change records nothing") {
        ParagraphIndents indents = buffer.lineIndents(0);
        buffer.commitParagraphIndents(0, indents, indents);
        REQUIRE_FALSE(buffer.canUndo());
    }
    SECTION("newline split inherits the right indent") {
        buffer.setLineIndents(0, {10, 0, 30});
        buffer.setCaret({0, 5});
        buffer.insertChar('\n');
        REQUIRE(buffer.lineCount() == 2);
        REQUIRE(buffer.lineIndents(1).right == 30);
        REQUIRE(buffer.lineIndents(1).left == 10);
    }
}

TEST_CASE("View menu lists Ruler", "[menu][ruler]") {
    auto menus = createMenuBarData();
    bool hasRuler = false;
    for (const auto& menu : menus) {
        if (menu.label == "View") {
            for (const auto& item : menu.items) {
                if (item.label == "Ruler") hasRuler = true;
            }
        }
    }
    REQUIRE(hasRuler);
}
