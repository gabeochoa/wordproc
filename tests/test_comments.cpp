#include "../src/editor/comments.h"

#include <ctime>

#include "catch2/catch.hpp"

namespace {

std::time_t makeTime(int year, int month, int day, int hour = 12) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

}  // namespace

TEST_CASE("Comment relative dates", "[comments]") {
    const std::time_t now = makeTime(2026, 10, 19);

    SECTION("same day is Today") {
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 19, 9), now) ==
                "Today");
    }
    SECTION("yesterday is Yesterday") {
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 18, 9), now) ==
                "Yesterday");
    }
    SECTION("a few days ago counts days") {
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 16, 9), now) ==
                "3 days ago");
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 14, 9), now) ==
                "5 days ago");
    }
    SECTION("a week or older shows the calendar date") {
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 12, 9), now) ==
                "10/12/2026");
    }
    SECTION("a future timestamp clamps to Today") {
        REQUIRE(comments::relativeDateLabel(makeTime(2026, 10, 20, 9), now) ==
                "Today");
    }
}

TEST_CASE("Comment leader line (paged mode)", "[comments]") {
    SECTION("spans the page edge to the card, 1px tall") {
        auto line = comments::leaderLine(820.0f, 828.0f, 221.0f);
        REQUIRE(line.present);
        REQUIRE(line.x == 820.0f);
        REQUIRE(line.y == 221.0f);
        REQUIRE(line.width == 8.0f);
        REQUIRE(line.height == 1.0f);
    }
    SECTION("no line when the card is clamped onto the page") {
        REQUIRE_FALSE(comments::leaderLine(820.0f, 821.0f, 221.0f).present);
        REQUIRE_FALSE(comments::leaderLine(820.0f, 820.0f, 221.0f).present);
        REQUIRE_FALSE(comments::leaderLine(828.0f, 820.0f, 221.0f).present);
    }
    SECTION("a 2px gap still draws the line") {
        auto line = comments::leaderLine(820.0f, 822.0f, 100.0f);
        REQUIRE(line.present);
        REQUIRE(line.width == 2.0f);
    }
}
