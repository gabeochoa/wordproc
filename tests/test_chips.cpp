#include "../src/editor/chips.h"

#include <ctime>

#include "catch2/catch.hpp"

namespace {

std::time_t makeTime(int year, int month, int day) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = 12;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

}  // namespace

TEST_CASE("Smart chip popup filtering", "[chips]") {
    const std::time_t now = makeTime(2026, 10, 19);

    SECTION("empty filter shows all four items in order") {
        auto items = chips::popupItems("", now);
        REQUIRE(items.size() == 4);
        REQUIRE(items[0].label == "Today — 10/19/2026");
        REQUIRE(items[1].label == "Tomorrow — 10/20/2026");
        REQUIRE(items[2].label == "Gabe Ochoa");
        REQUIRE(items[3].label == "Front Door (AI)");
    }
    SECTION("@ga matches only the person") {
        auto items = chips::popupItems("ga", now);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].label == "Gabe Ochoa");
        REQUIRE(items[0].kind == ChipKind::Person);
    }
    SECTION("@to matches both dates") {
        auto items = chips::popupItems("to", now);
        REQUIRE(items.size() == 2);
        REQUIRE(items[0].label == "Today — 10/19/2026");
        REQUIRE(items[1].label == "Tomorrow — 10/20/2026");
    }
    SECTION("filtering is case-insensitive") {
        auto items = chips::popupItems("GABE", now);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].label == "Gabe Ochoa");
    }
    SECTION("no match gives an empty list") {
        REQUIRE(chips::popupItems("zzz", now).empty());
    }
    SECTION("date labels follow the given day") {
        REQUIRE(chips::todayLabel(makeTime(2026, 1, 5)) ==
                "Today — 1/5/2026");
        REQUIRE(chips::tomorrowLabel(makeTime(2026, 12, 31)) ==
                "Tomorrow — 1/1/2027");
    }
}

TEST_CASE("Smart chip insert/delete", "[chips]") {
    SECTION("insert replaces @ + filter with the label, one undo step") {
        TextBuffer buffer;
        buffer.setText("Meet @to");
        std::vector<Chip> chipList;
        REQUIRE(chips::insertChipAt(buffer, chipList, 5, 8, ChipKind::Date,
                                    "Today — 10/19/2026"));
        REQUIRE(buffer.getText() == "Meet Today — 10/19/2026");
        REQUIRE(chipList.size() == 1);
        REQUIRE(chipList[0].startOffset == 5);
        // "Today — 10/19/2026" is 20 bytes: the em-dash is 3 UTF-8 bytes.
        REQUIRE(chipList[0].endOffset == 5 + 20);
        REQUIRE(chipList[0].label == "Today — 10/19/2026");
        REQUIRE(chips::bufferMatches(buffer, chipList[0]));
        REQUIRE(buffer.canUndo());
        buffer.undo();
        REQUIRE(buffer.getText() == "Meet @to");
    }
    SECTION("insert re-anchors a later chip") {
        TextBuffer buffer;
        buffer.setText("@a @b");
        std::vector<Chip> chipList;
        REQUIRE(chips::insertChipAt(buffer, chipList, 3, 5, ChipKind::Person,
                                    "Gabe Ochoa"));
        REQUIRE(buffer.getText() == "@a Gabe Ochoa");
        REQUIRE(chips::insertChipAt(buffer, chipList, 0, 2, ChipKind::Person,
                                    "Gabe Ochoa"));
        REQUIRE(buffer.getText() == "Gabe Ochoa Gabe Ochoa");
        REQUIRE(chipList.size() == 2);
        // The second chip moved right by the first insert's delta (+8).
        REQUIRE(chipList[0].startOffset == 3 + 8);
        REQUIRE(chipList[1].startOffset == 0);
    }
    SECTION("backspace at chip end deletes the whole chip") {
        TextBuffer buffer;
        buffer.setText("Meet Today — 10/19/2026!");
        std::vector<Chip> chipList;
        chipList.push_back(
            Chip{5, 25, ChipKind::Date, "Today — 10/19/2026"});
        REQUIRE(chips::chipEndingAt(buffer, chipList, 25) == 0);
        REQUIRE(chips::chipEndingAt(buffer, chipList, 24) == -1);
        REQUIRE(chips::eraseChipAt(buffer, chipList, 0));
        REQUIRE(buffer.getText() == "Meet !");
        REQUIRE(chipList.empty());
    }
    SECTION("an edit inside a chip orphans it (render skips it)") {
        TextBuffer buffer;
        buffer.setText("Meet Today — 10/19/2026");
        std::vector<Chip> chipList;
        chipList.push_back(
            Chip{5, 25, ChipKind::Date, "Today — 10/19/2026"});
        // Simulate an edit inside the chip: replace one char.
        buffer.replaceRange(6, 1, "X");
        REQUIRE_FALSE(chips::bufferMatches(buffer, chipList[0]));
        REQUIRE(chips::chipEndingAt(buffer, chipList, 25) == -1);
    }
    SECTION("invalid ranges are rejected") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Chip> chipList;
        REQUIRE_FALSE(
            chips::insertChipAt(buffer, chipList, 5, 6, ChipKind::Person,
                                "Gabe Ochoa"));
        REQUIRE_FALSE(chips::eraseChipAt(buffer, chipList, 0));
    }
}
