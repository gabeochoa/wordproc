#include "../src/editor/scroll_math.h"
#include "catch2/catch.hpp"

TEST_CASE("scroll wheel accumulation", "[scroll]") {
    SECTION("small deltas sum to whole lines") {
        float remainder = 0.0f;
        int total = 0;
        // Ten 0.1-notch downward events: each is 0.3 lines, total 3 lines.
        for (int i = 0; i < 10; ++i) {
            total += scroll_math::accumulateWheel(remainder, -0.1f);
        }
        REQUIRE(total == 3);
        REQUIRE(remainder == 0.0f);
    }

    SECTION("a single small delta moves nothing but keeps the remainder") {
        float remainder = 0.0f;
        REQUIRE(scroll_math::accumulateWheel(remainder, -0.1f) == 0);
        REQUIRE(remainder == Approx(0.3f));
    }

    SECTION("large flick applies at once") {
        float remainder = 0.0f;
        REQUIRE(scroll_math::accumulateWheel(remainder, -10.0f) == 30);
        REQUIRE(remainder == 0.0f);
    }

    SECTION("upward scrolling accumulates symmetrically") {
        float remainder = 0.0f;
        int total = 0;
        for (int i = 0; i < 10; ++i) {
            total += scroll_math::accumulateWheel(remainder, 0.1f);
        }
        REQUIRE(total == -3);
        REQUIRE(remainder == 0.0f);
    }

    SECTION("mixed directions cancel") {
        float remainder = 0.0f;
        REQUIRE(scroll_math::accumulateWheel(remainder, -0.1f) == 0);
        REQUIRE(scroll_math::accumulateWheel(remainder, 0.1f) == 0);
        REQUIRE(remainder == 0.0f);
    }
}

TEST_CASE("scrollbar geometry", "[scroll]") {
    SECTION("thumb sized viewport over total with offset mapping") {
        auto g = scroll_math::scrollbarGeometry(100.0f, 400.0f, 100, 20, 80, 0);
        REQUIRE(g.thumbVisible);
        REQUIRE(g.trackHeight == 368.0f);
        REQUIRE(g.thumbHeight == Approx(368.0f * 0.2f));
        REQUIRE(g.thumbY == Approx(g.trackY));
        auto bottom = scroll_math::scrollbarGeometry(100.0f, 400.0f, 100, 20, 80, 80);
        REQUIRE(bottom.thumbY == Approx(bottom.trackY + bottom.trackHeight - bottom.thumbHeight));
        // Round-trip: offset at the bottom maps back to maxScroll.
        REQUIRE(scroll_math::offsetForThumbY(bottom.thumbY, bottom.trackY,
                                             bottom.trackHeight, bottom.thumbHeight, 80) == 80);
        // Round-trip at the top.
        REQUIRE(scroll_math::offsetForThumbY(g.thumbY, g.trackY, g.trackHeight, g.thumbHeight, 80) == 0);
    }

    SECTION("thumb has a minimum size") {
        auto g = scroll_math::scrollbarGeometry(0.0f, 400.0f, 10000, 20, 9980, 0);
        REQUIRE(g.thumbHeight == 20.0f);
        REQUIRE(g.thumbVisible);
    }

    SECTION("no thumb when everything fits") {
        auto g = scroll_math::scrollbarGeometry(0.0f, 400.0f, 10, 20, 0, 0);
        REQUIRE_FALSE(g.thumbVisible);
    }

    SECTION("thumb midpoint maps to half the scroll range") {
        auto g = scroll_math::scrollbarGeometry(0.0f, 400.0f, 100, 20, 80, 40);
        int back = scroll_math::offsetForThumbY(g.thumbY, g.trackY, g.trackHeight, g.thumbHeight, 80);
        REQUIRE(back == 40);
    }
}
