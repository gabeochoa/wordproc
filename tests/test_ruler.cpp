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
    SECTION("scaled geometry: stored 72 lands on the 1-inch tick") {
        // Paged fit ~0.5: 36 px/inch. A stored 1\" indent (72) must
        // draw at origin+36 and drag back to 72 — not origin+72.
        auto scaled = ruler_math::geometry(123.0f, 20.0f, 450.0f, 300.0f,
                                            0.0f, 36.0f);
        REQUIRE(scaled.pixelsPerInch == 36.0f);
        REQUIRE(scaled.indentScale() == 0.5f);
        REQUIRE(ruler_math::leftMarkerX(scaled, 72) == 458.0f + 36.0f);
        REQUIRE(ruler_math::firstLineMarkerX(scaled, 72, 36) ==
                458.0f + 54.0f);
        REQUIRE(ruler_math::rightMarkerX(scaled, 72) ==
                458.0f + scaled.textSpanWidth - 36.0f);
        REQUIRE(ruler_math::indentFromX(scaled, 458.0f + 36.0f) == 72);
        REQUIRE(ruler_math::firstLineIndentFromX(scaled, 458.0f + 54.0f,
                                                  72) == 36);
        REQUIRE(ruler_math::rightIndentFromX(
                    scaled, 458.0f + scaled.textSpanWidth - 36.0f) == 72);
        // Clamp is in stored units: dragging past the right edge
        // yields the span in stored units, not screen pixels.
        REQUIRE(ruler_math::indentFromX(scaled, 100000.0f) ==
                static_cast<int>(std::lround(scaled.textSpanWidth / 0.5f)));
    }
    SECTION("marker shapes point toward the centre (hourglass)") {
        auto g = ruler_math::geometry(123.0f, 20.0f, 3.0f, 1274.0f, 0.0f);
        // rulerY = 103, height 20, mid = 113.
        auto s = ruler_math::markerShapes(g, 72, 36, 48);
        // First-line (top): apex down at mid-1, base at the top.
        REQUIRE(s.firstLine.apexX == 11.0f + 72.0f + 36.0f);
        REQUIRE(s.firstLine.apexY == 112.0f);
        REQUIRE(s.firstLine.baseY == 104.0f);
        // Left / hanging (bottom): apex up at mid+1, base above the
        // rectangle, rectangle at the band's bottom edge.
        REQUIRE(s.left.apexX == 11.0f + 72.0f);
        REQUIRE(s.left.apexY == 114.0f);
        REQUIRE(s.left.baseY == 120.0f);
        REQUIRE(s.rectX == 11.0f + 72.0f - 3.0f);
        REQUIRE(s.rectY == 120.0f);
        REQUIRE(s.rectW == 6.0f);
        REQUIRE(s.rectH == 2.0f);
        // Right: apex up at mid+1, base at the bottom edge.
        REQUIRE(s.right.apexX == 11.0f + g.textSpanWidth - 48.0f);
        REQUIRE(s.right.apexY == 114.0f);
        REQUIRE(s.right.baseY == 122.0f);
        // At firstLine == 0 the pair forms an hourglass: both apexes
        // share the left-indent x.
        auto aligned = ruler_math::markerShapes(g, 72, 0, 0);
        REQUIRE(aligned.firstLine.apexX == aligned.left.apexX);
    }
    SECTION("page width is 8.5 * pixelsPerInch at any scale") {
        // The layout layer's contract: pageDisplayWidth =
        // pageWidth(points) * displayScale = 8.5 * ppi. Here the
        // ruler half: 8.5 inches at 36 px/in spans 306 px.
        auto scaled = ruler_math::geometry(123.0f, 20.0f, 450.0f, 300.0f,
                                            0.0f, 36.0f);
        REQUIRE(8.5f * scaled.pixelsPerInch == 306.0f);
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

TEST_CASE("scaled document font follows displayScale", "[ruler]") {
    // Paged fit ~0.58 (ruler 42px/in): the 16px base renders at 9px
    // with a 13px line height, so text sits in the page's scale
    // instead of overflowing the 6.5" text width at full size.
    constexpr float paged = 42.0f / 72.0f;  // ~0.5833
    SECTION("paged fit scales the base font down") {
        REQUIRE(ruler_math::scaledFontSize(16, paged) == 9);
        REQUIRE(ruler_math::scaledLineHeight(16, paged) == 13);
        REQUIRE(ruler_math::scaledParagraphFontSize(16, paged) == 9);
    }
    SECTION("headings scale by the same factor") {
        REQUIRE(ruler_math::scaledParagraphFontSize(28, paged) == 16);
        // Paragraph path truncates (11.67 -> 11), it does not round.
        REQUIRE(ruler_math::scaledParagraphFontSize(20, paged) == 11);
    }
    SECTION("pageless displayScale == zoomLevel, so output is unchanged") {
        REQUIRE(ruler_math::scaledFontSize(16, 1.0f) == 16);
        REQUIRE(ruler_math::scaledLineHeight(16, 1.0f) == 20);
        REQUIRE(ruler_math::scaledParagraphFontSize(16, 1.0f) == 16);
        REQUIRE(ruler_math::scaledFontSize(16, 1.5f) == 24);
        REQUIRE(ruler_math::scaledParagraphFontSize(16, 1.5f) == 24);
    }
    SECTION("the pre-existing 8px floor is preserved") {
        // A 10px base at the paged fit would round to 6; the floor
        // keeps it at 8, exactly as pageless zoom did before.
        REQUIRE(ruler_math::scaledFontSize(10, paged) == 8);
        REQUIRE(ruler_math::scaledFontSize(8, paged) == 8);
        REQUIRE(ruler_math::scaledLineHeight(10, paged) == 12);
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

TEST_CASE("Ruler column white spans", "[ruler]") {
    // Effective text area x 435, width 650: origin 443, span 634.
    // 2 columns, spacing 36: col width (634 - 36) / 2 = 299; spans
    // [443, 742] and [778, 1077]; the 36px gutter [742, 778] stays gray.
    auto g = ruler_math::geometry(123.0f, 20.0f, 435.0f, 650.0f, 0.0f);
    auto spans = ruler_math::columnWhiteSpans(g, 2, 36.0f);
    REQUIRE(spans.size() == 2);
    REQUIRE(spans[0].first == 443.0f);
    REQUIRE(spans[0].second == 742.0f);
    REQUIRE(spans[1].first == 778.0f);
    REQUIRE(spans[1].second == 1077.0f);
    // Single column: one span, the whole white band.
    auto single = ruler_math::columnWhiteSpans(g, 1, 36.0f);
    REQUIRE(single.size() == 1);
    REQUIRE(single[0].first == 435.0f);
    REQUIRE(single[0].second == 1085.0f);
}
