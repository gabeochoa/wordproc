#include "../src/editor/scroll_math.h"

#include "catch2/catch.hpp"

// Print Preview pagination is an ESTIMATE (no real pagination exists):
// lines-per-page from page text height / line height, page count from
// ceil(total / per-page). These pin the shared helpers the toolbar,
// renderer stride, and preview entry all use.

TEST_CASE("Print Preview pagination - lines per page", "[print][preview]") {
    SECTION("fits floor(height / lineHeight) lines") {
        REQUIRE(scroll_math::previewLinesPerPage(300.0f, 20.0f) == 15);
        REQUIRE(scroll_math::previewLinesPerPage(301.0f, 20.0f) == 15);
        REQUIRE(scroll_math::previewLinesPerPage(319.0f, 20.0f) == 15);
        REQUIRE(scroll_math::previewLinesPerPage(320.0f, 20.0f) == 16);
    }
    SECTION("degenerate inputs yield at least one line per page") {
        REQUIRE(scroll_math::previewLinesPerPage(0.0f, 20.0f) == 1);
        REQUIRE(scroll_math::previewLinesPerPage(300.0f, 0.0f) == 1);
        REQUIRE(scroll_math::previewLinesPerPage(-5.0f, -5.0f) == 1);
        REQUIRE(scroll_math::previewLinesPerPage(10.0f, 20.0f) == 1);
    }
}

TEST_CASE("Print Preview pagination - page count", "[print][preview]") {
    SECTION("exact multiples need no extra page") {
        REQUIRE(scroll_math::previewPageCount(0, 25) == 1);
        REQUIRE(scroll_math::previewPageCount(25, 25) == 1);
        REQUIRE(scroll_math::previewPageCount(50, 25) == 2);
    }
    SECTION("a partial last page still counts as a page") {
        REQUIRE(scroll_math::previewPageCount(1, 25) == 1);
        REQUIRE(scroll_math::previewPageCount(26, 25) == 2);
        REQUIRE(scroll_math::previewPageCount(51, 25) == 3);
    }
    SECTION("degenerate per-page falls back to one line per page") {
        REQUIRE(scroll_math::previewPageCount(5, 0) == 5);
        REQUIRE(scroll_math::previewPageCount(5, -3) == 5);
    }
}

TEST_CASE("Print Preview pagination - first line and clamp", "[print][preview]") {
    SECTION("first line is page * linesPerPage") {
        REQUIRE(scroll_math::previewFirstLine(0, 25) == 0);
        REQUIRE(scroll_math::previewFirstLine(1, 25) == 25);
        REQUIRE(scroll_math::previewFirstLine(2, 25) == 50);
    }
    SECTION("clamp keeps the page in [0, count-1]") {
        REQUIRE(scroll_math::previewPageClamp(-1, 3) == 0);
        REQUIRE(scroll_math::previewPageClamp(0, 3) == 0);
        REQUIRE(scroll_math::previewPageClamp(2, 3) == 2);
        REQUIRE(scroll_math::previewPageClamp(3, 3) == 2);
        REQUIRE(scroll_math::previewPageClamp(99, 3) == 2);
        REQUIRE(scroll_math::previewPageClamp(5, 1) == 0);
    }
}
