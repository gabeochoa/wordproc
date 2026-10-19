#include "../src/editor/version_history.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "catch2/catch.hpp"

namespace {

std::time_t makeTime(int year, int month, int day, int hour, int min,
                     int sec) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

// Unique temp dir per test case, removed at the end (RAII).
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        path = std::filesystem::temp_directory_path() /
               ("wordproc_versions_test_" +
                std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch()
                        .count()));
        std::filesystem::create_directories(path);
    }
    ~TempDir() { std::filesystem::remove_all(path); }
};

}  // namespace

TEST_CASE("Version history naming and index", "[version_history]") {
    SECTION("timestamp label is YYYYmmdd-HHMMSS") {
        REQUIRE(version_history::timestampLabel(
                    makeTime(2026, 10, 19, 9, 5, 7)) == "20261019-090507");
    }
    SECTION("doc names are sanitized, never escape the parent") {
        REQUIRE(version_history::sanitizeDocName("My Doc") == "My_Doc");
        REQUIRE(version_history::sanitizeDocName("../evil") == ".._evil");
        REQUIRE(version_history::sanitizeDocName("") == "Untitled");
        REQUIRE(version_history::sanitizeDocName("...") == "Untitled");
        REQUIRE(version_history::sanitizeDocName("report.v2") == "report.v2");
    }
    SECTION("index line carries time, words, chars, file") {
        REQUIRE(version_history::indexLine("20261019-090507", 3, 14,
                                           "20261019-090507.txt") ==
                "20261019-090507 words=3 chars=14 file=20261019-090507.txt");
    }
    SECTION("human row labels parse the local timestamp stem") {
        REQUIRE(version_history::displayLabel("20261018-212000", 128) ==
                "Oct 18, 2026, 9:20 PM — 128 words");
        REQUIRE(version_history::displayLabel("20261019-000500", 1) ==
                "Oct 19, 2026, 12:05 AM — 1 word");
        REQUIRE(version_history::displayLabel("20260105-120000", 0) ==
                "Jan 5, 2026, 12:00 PM — 0 words");
        // Same-second "-1" suffix is ignored.
        REQUIRE(version_history::displayLabel("20261018-212000-1", 3) ==
                "Oct 18, 2026, 9:20 PM — 3 words");
        // Malformed stems fall back to the raw stem.
        REQUIRE(version_history::displayLabel("garbage", 2) ==
                "garbage — 2 words");
        REQUIRE(version_history::displayLabel("20261399-999999", 2) ==
                "20261399-999999 — 2 words");
    }
    SECTION("word counting matches the stats rule") {
        REQUIRE(version_history::countWords("One two three.") == 3);
        REQUIRE(version_history::countWords("") == 0);
        REQUIRE(version_history::countWords("  spaced   out  ") == 2);
    }
}

#ifndef _WIN32
namespace {

// RAII TZ switch (same pattern as test_chips.cpp): restores the
// previous TZ on destruction, so a failing REQUIRE cannot leak
// the zone into later test cases.
struct ScopedTZ {
    std::string old_;
    bool had_ = false;
    explicit ScopedTZ(const char* tz) {
        const char* cur = std::getenv("TZ");
        if (cur != nullptr) {
            old_ = cur;
            had_ = true;
        }
        setenv("TZ", tz, 1);
        tzset();
    }
    ~ScopedTZ() {
        if (had_) {
            setenv("TZ", old_.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
    }
};

}  // namespace

TEST_CASE("Version history timestamps use LOCAL time, not UTC",
          "[version_history]") {
    // 2026-10-19 01:30 UTC is still 2026-10-18 21:30 in
    // America/New_York (EDT, UTC-4). A UTC-based stamp would say
    // 20261019-013000; the local stamp says 20261018-213000.
    // Same instant the chip test pins — both surfaces must agree
    // on OS-local time (no hardcoded zone anywhere).
    std::time_t instant = 0;
    {
        ScopedTZ utc("UTC");
        std::tm tm{};
        tm.tm_year = 2026 - 1900;
        tm.tm_mon = 9;
        tm.tm_mday = 19;
        tm.tm_hour = 1;
        tm.tm_min = 30;
        tm.tm_isdst = 0;
        instant = std::mktime(&tm);
    }
    {
        ScopedTZ eastern("America/New_York");
        REQUIRE(version_history::timestampLabel(instant) ==
                "20261018-213000");
    }
    // No gmtime anywhere: the stamp is localtime_r/s in
    // version_history.h, as chips are in chips.h.
}

#endif

TEST_CASE("Version history snapshot write/list", "[version_history]") {
    TempDir tmp;
    const std::time_t t1 = makeTime(2026, 10, 19, 9, 0, 0);
    const std::time_t t2 = makeTime(2026, 10, 19, 9, 5, 0);

    SECTION("write + list newest-first with counts") {
        std::string p1 = version_history::writeSnapshot(tmp.path, "Doc",
                                                        "One two", t1);
        std::string p2 = version_history::writeSnapshot(tmp.path, "Doc",
                                                        "One two three.", t2);
        REQUIRE(!p1.empty());
        REQUIRE(!p2.empty());
        REQUIRE(p1 != p2);
        auto snaps = version_history::listSnapshots(tmp.path / "Doc");
        REQUIRE(snaps.size() == 2);
        REQUIRE(snaps[0].label == "20261019-090500");
        REQUIRE(snaps[0].words == 3);
        REQUIRE(snaps[0].chars == 14);
        REQUIRE(snaps[1].label == "20261019-090000");
        REQUIRE(snaps[1].words == 2);
        // The index log has one line per snapshot.
        std::ifstream index(tmp.path / "Doc" / "index.txt");
        std::stringstream buf;
        buf << index.rdbuf();
        REQUIRE(buf.str().find("20261019-090000 words=2") !=
                std::string::npos);
        REQUIRE(buf.str().find("20261019-090500 words=3") !=
                std::string::npos);
        // Snapshots round-trip their text.
        REQUIRE(version_history::readSnapshot(snaps[0].path) ==
                "One two three.");
    }
    SECTION("same-second saves never overwrite each other") {
        std::string p1 = version_history::writeSnapshot(tmp.path, "Doc",
                                                        "first", t1);
        std::string p2 = version_history::writeSnapshot(tmp.path, "Doc",
                                                        "second", t1);
        REQUIRE(p1 != p2);
        REQUIRE(p2.find("-1.txt") != std::string::npos);
        auto snaps = version_history::listSnapshots(tmp.path / "Doc");
        REQUIRE(snaps.size() == 2);
    }
    SECTION("missing dir lists empty") {
        REQUIRE(version_history::listSnapshots(tmp.path / "Nope").empty());
    }
    SECTION("index.txt itself is not listed as a snapshot") {
        version_history::writeSnapshot(tmp.path, "Doc", "hi", t1);
        auto snaps = version_history::listSnapshots(tmp.path / "Doc");
        REQUIRE(snaps.size() == 1);
    }
}
