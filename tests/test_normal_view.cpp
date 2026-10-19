#include "../src/editor/columns_math.h"
#include "../src/editor/normal_view.h"
#include "../src/editor/scroll_math.h"

#include "catch2/catch.hpp"

// View > Normal: dashed computed page-break lines. Boundaries are
// the preview estimate (page text height / line height): every
// linesPerPage-th buffer row, never row 0.

TEST_CASE("Normal view page boundaries", "[normal][view]") {
    SECTION("row 0 never starts a boundary") {
        REQUIRE_FALSE(normal_view::isPageBoundary(0, 32));
        REQUIRE_FALSE(normal_view::isPageBoundary(0, 1));
    }
    SECTION("every linesPerPage-th row is a boundary") {
        REQUIRE(normal_view::isPageBoundary(32, 32));
        REQUIRE(normal_view::isPageBoundary(64, 32));
        REQUIRE(normal_view::isPageBoundary(96, 32));
        REQUIRE_FALSE(normal_view::isPageBoundary(31, 32));
        REQUIRE_FALSE(normal_view::isPageBoundary(33, 32));
        REQUIRE_FALSE(normal_view::isPageBoundary(63, 32));
    }
    SECTION("degenerate lines-per-page means no boundaries") {
        REQUIRE_FALSE(normal_view::isPageBoundary(32, 0));
        REQUIRE_FALSE(normal_view::isPageBoundary(32, -5));
    }
    SECTION("one line per page puts a boundary above every row but the first") {
        REQUIRE_FALSE(normal_view::isPageBoundary(0, 1));
        REQUIRE(normal_view::isPageBoundary(1, 1));
        REQUIRE(normal_view::isPageBoundary(7, 1));
    }
}

TEST_CASE("Normal view break line sits between rows", "[normal][view]") {
    SECTION("default gap: one pixel above the new row's top edge") {
        // Previous row top 540, font 16: painted bottom 540 + 16
        // + 2 = 558 (glyphs + squiggle). New row top 560: gap 2,
        // line at 560 - 1 = 559 — below the squiggle, above the
        // new row's glyphs. The old formula (top - 2 = 558) drew
        // exactly on the squiggle row.
        REQUIRE(normal_view::breakLineY(560, 558) == 559);
    }
    SECTION("wider gap (paragraph spacing): midpoint of the gap") {
        REQUIRE(normal_view::breakLineY(586, 578) == 582);
        REQUIRE(normal_view::breakLineY(570, 558) == 564);
    }
    SECTION("odd gap rounds toward the new row's top edge") {
        REQUIRE(normal_view::breakLineY(581, 580) == 581);
        REQUIRE(normal_view::breakLineY(583, 580) == 582);
    }
    SECTION("degenerate gap or no previous row: one pixel above") {
        REQUIRE(normal_view::breakLineY(580, 580) == 579);
        REQUIRE(normal_view::breakLineY(580, 590) == 579);
        REQUIRE(normal_view::breakLineY(580, -1) == 579);
    }
}

TEST_CASE("Normal view boundary count", "[normal][view]") {
    SECTION("boundaries strictly below the last row") {
        // 70 rows at 32/page: boundaries at rows 32 and 64.
        REQUIRE(normal_view::pageBoundaryCount(70, 32) == 2);
        // Exactly one page: no boundary (row 32 is not a row).
        REQUIRE(normal_view::pageBoundaryCount(32, 32) == 0);
        // One row past a page: the boundary at row 32 exists.
        REQUIRE(normal_view::pageBoundaryCount(33, 32) == 1);
        REQUIRE(normal_view::pageBoundaryCount(0, 32) == 0);
        REQUIRE(normal_view::pageBoundaryCount(1, 32) == 0);
    }
    SECTION("degenerate lines-per-page means no boundaries") {
        REQUIRE(normal_view::pageBoundaryCount(70, 0) == 0);
        REQUIRE(normal_view::pageBoundaryCount(70, -1) == 0);
    }
}

TEST_CASE("Normal view lines-per-page is the preview estimate",
          "[normal][preview]") {
    SECTION("Letter, 1in margins, 16px font: 32 lines per page") {
        // (792 - 72 - 72) / (16 + 4) = 648 / 20 = 32.4 -> 32.
        // The renderer (columnsLinesPerPage) and the preview
        // (previewLinesPerPage) compute this identically; pin both
        // helpers so a drift in either shows up here.
        REQUIRE(columns_math::linesPerColumnForPage(792.0f, 72.0f, 72.0f,
                                                     1.0f, 20) == 32);
        REQUIRE(scroll_math::previewLinesPerPage(648.0f, 20.0f) == 32);
    }
    SECTION("landscape Letter, 1in margins: same text height, same count") {
        // Landscape swaps width/height; the page TEXT height is
        // (612 - 144) = 468 -> 468 / 20 = 23.4 -> 23.
        REQUIRE(columns_math::linesPerColumnForPage(612.0f, 72.0f, 72.0f,
                                                     1.0f, 20) == 23);
    }
}
