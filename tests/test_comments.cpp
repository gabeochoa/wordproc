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
