#pragma once

// Margin comments (Docs-era, Win95-styled): pure helpers for the
// Comment type in document_settings.h. The render/cards live in
// render_system.h (anchor highlight) and MenuUISystem (cards).

#include <cstdio>
#include <ctime>
#include <string>

namespace comments {

// Relative date for a comment card header: "Today", "Yesterday",
// "N days ago" (2..6), else the calendar date M/d/YYYY. Pure over
// (createdAt, now) so tests pin it with fixed times; both args are
// seconds since epoch in local time.
inline std::string relativeDateLabel(std::time_t createdAt,
                                     std::time_t now) {
    if (now < createdAt) now = createdAt;
    const double days =
        std::difftime(now, createdAt) / (60.0 * 60.0 * 24.0);
    if (days < 1.0) return "Today";
    if (days < 2.0) return "Yesterday";
    if (days < 7.0) {
        return std::to_string(static_cast<int>(days)) + " days ago";
    }
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &createdAt);
#else
    localtime_r(&createdAt, &tm);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d/%d/%d", tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_year + 1900);
    return buf;
}

}  // namespace comments
