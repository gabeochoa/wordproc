// Unit tests for the Go To math (Batch 9, Edit > Go To...):
// the number parser, page first-line, and line clamping
// in src/editor/goto.h. The live jump (cmd::performGoTo)
// applies these to the buffer and is covered E2E by
// pass_goto_dialog.e2e (Ctrl+G binding included: the
// ActionMap preset is not linkable into this harness).
#include "../src/editor/goto.h"

#include "catch2/catch.hpp"

TEST_CASE("Go To number parsing", "[goto]") {
    REQUIRE(goto_math::parseGoToNumber("1") == 1);
    REQUIRE(goto_math::parseGoToNumber("50") == 50);
    REQUIRE(goto_math::parseGoToNumber("  7  ") == 7);
    REQUIRE(goto_math::parseGoToNumber("") == -1);
    REQUIRE(goto_math::parseGoToNumber("   ") == -1);
    REQUIRE(goto_math::parseGoToNumber("0") == -1);
    REQUIRE(goto_math::parseGoToNumber("abc") == -1);
    REQUIRE(goto_math::parseGoToNumber("12a") == -1);
    REQUIRE(goto_math::parseGoToNumber("1.5") == -1);
    REQUIRE(goto_math::parseGoToNumber("-3") == -1);
    REQUIRE(goto_math::parseGoToNumber("99999999999") == -1);
}

TEST_CASE("Go To page first line", "[goto]") {
    // 100 lines, 32 per page: pages 1..4 (last page starts 96).
    REQUIRE(goto_math::firstLineOfPage(100, 32, 1) == 0);
    REQUIRE(goto_math::firstLineOfPage(100, 32, 2) == 32);
    REQUIRE(goto_math::firstLineOfPage(100, 32, 4) == 96);
    SECTION("page past the end clamps to the last page's first line") {
        REQUIRE(goto_math::firstLineOfPage(100, 32, 999) == 96);
    }
    SECTION("page past the end on a short doc clamps to the last line") {
        // 10 lines, 32 per page: one page, first line 0.
        REQUIRE(goto_math::firstLineOfPage(10, 32, 999) == 0);
        // 40 lines, 32 per page: page 2 starts at 32, the
        // last line is 39 — the page start is in range.
        REQUIRE(goto_math::firstLineOfPage(40, 32, 999) == 32);
    }
    SECTION("degenerate inputs") {
        REQUIRE(goto_math::firstLineOfPage(0, 32, 1) == 0);
        REQUIRE(goto_math::firstLineOfPage(100, 0, 2) == 1);
        REQUIRE(goto_math::firstLineOfPage(100, 32, 0) == 0);
        REQUIRE(goto_math::firstLineOfPage(100, 32, -5) == 0);
    }
}

TEST_CASE("Go To line clamping", "[goto]") {
    REQUIRE(goto_math::clampedLine(100, 1) == 0);
    REQUIRE(goto_math::clampedLine(100, 50) == 49);
    REQUIRE(goto_math::clampedLine(100, 100) == 99);
    SECTION("line past the end clamps to the last line") {
        REQUIRE(goto_math::clampedLine(100, 999) == 99);
    }
    SECTION("degenerate inputs") {
        REQUIRE(goto_math::clampedLine(0, 1) == 0);
        REQUIRE(goto_math::clampedLine(100, 0) == 0);
        REQUIRE(goto_math::clampedLine(100, -5) == 0);
    }
}
