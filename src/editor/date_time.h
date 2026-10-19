#pragma once

// Insert > Date and Time... format functions (Word 95 parity).
// Pure functions over std::tm so unit tests can pin them with a fixed
// time; the dialog computes the list from the current local time at
// open. Month names are English literals (the Word 95 dialog is an
// English UI), not strftime %B/%b, so the output is locale-independent
// and the slash format matches Word's M/d/yyyy (no zero padding).

#include <array>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace date_time {

// Number of formats in the dialog list (order is the dialog order).
inline constexpr int FORMAT_COUNT = 6;

inline std::string formatDateTime(const std::tm& tm, int index) {
    static constexpr std::array<const char*, 12> kMonths = {
        "January", "February", "March",     "April",
        "May",     "June",     "July",      "August",
        "September", "October", "November",  "December"};
    static constexpr std::array<const char*, 12> kMonthsShort = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const int month = tm.tm_mon >= 0 && tm.tm_mon < 12 ? tm.tm_mon : 0;
    const int year = tm.tm_year + 1900;
    char buf[64];
    switch (index) {
        case 0:  // 10/18/2026
            std::snprintf(buf, sizeof(buf), "%d/%d/%d", month + 1,
                          tm.tm_mday, year);
            return buf;
        case 1:  // October 18, 2026
            std::snprintf(buf, sizeof(buf), "%s %d, %d", kMonths[month],
                          tm.tm_mday, year);
            return buf;
        case 2:  // Oct 18, 2026
            std::snprintf(buf, sizeof(buf), "%s %d, %d", kMonthsShort[month],
                          tm.tm_mday, year);
            return buf;
        case 3: {  // 6:06 PM
            int hour12 = tm.tm_hour % 12;
            if (hour12 == 0) hour12 = 12;
            std::snprintf(buf, sizeof(buf), "%d:%02d %s", hour12, tm.tm_min,
                          tm.tm_hour < 12 ? "AM" : "PM");
            return buf;
        }
        case 4:  // 18 October 2026
            std::snprintf(buf, sizeof(buf), "%d %s %d", tm.tm_mday,
                          kMonths[month], year);
            return buf;
        case 5:  // 2026-10-18 (ISO date)
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year,
                          month + 1, tm.tm_mday);
            return buf;
        default:
            return "";
    }
}

// All dialog formats for a broken-down time, in dialog order.
inline std::vector<std::string> dateTimeFormats(const std::tm& tm) {
    std::vector<std::string> formats;
    formats.reserve(FORMAT_COUNT);
    for (int i = 0; i < FORMAT_COUNT; ++i) {
        formats.push_back(formatDateTime(tm, i));
    }
    return formats;
}

// Current local time as a broken-down time.
inline std::tm localTimeNow() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    return tm;
}

}  // namespace date_time
