#include "../src/editor/date_time.h"

#include "catch2/catch.hpp"

TEST_CASE("Date/Time formats with a fixed time", "[date_time]") {
    std::tm tm{};
    tm.tm_year = 126;  // 2026
    tm.tm_mon = 9;     // October
    tm.tm_mday = 18;
    tm.tm_hour = 18;
    tm.tm_min = 6;

    auto formats = date_time::dateTimeFormats(tm);
    REQUIRE(formats.size() == 6);
    REQUIRE(formats[0] == "10/18/2026");
    REQUIRE(formats[1] == "October 18, 2026");
    REQUIRE(formats[2] == "Oct 18, 2026");
    REQUIRE(formats[3] == "6:06 PM");
    REQUIRE(formats[4] == "18 October 2026");
    REQUIRE(formats[5] == "2026-10-18");
}

TEST_CASE("Date/Time formats at day/month/hour edges", "[date_time]") {
    std::tm tm{};
    tm.tm_year = 126;  // 2026
    tm.tm_mon = 0;     // January
    tm.tm_mday = 5;
    tm.tm_hour = 0;
    tm.tm_min = 5;

    // Word-style: no zero padding on month/day in the slash and long
    // formats (1/5/2026, January 5, 2026); ISO keeps padding.
    REQUIRE(date_time::formatDateTime(tm, 0) == "1/5/2026");
    REQUIRE(date_time::formatDateTime(tm, 1) == "January 5, 2026");
    REQUIRE(date_time::formatDateTime(tm, 2) == "Jan 5, 2026");
    REQUIRE(date_time::formatDateTime(tm, 3) == "12:05 AM");
    REQUIRE(date_time::formatDateTime(tm, 4) == "5 January 2026");
    REQUIRE(date_time::formatDateTime(tm, 5) == "2026-01-05");

    tm.tm_hour = 12;
    tm.tm_min = 0;
    REQUIRE(date_time::formatDateTime(tm, 3) == "12:00 PM");

    tm.tm_hour = 23;
    tm.tm_min = 59;
    REQUIRE(date_time::formatDateTime(tm, 3) == "11:59 PM");

    REQUIRE(date_time::formatDateTime(tm, 99).empty());
}
