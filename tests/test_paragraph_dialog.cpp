#include "../src/editor/paragraph_dialog.h"

#include "catch2/catch.hpp"

TEST_CASE("Paragraph dialog field conversions", "[paragraph_dialog]") {
    using namespace paragraph_dialog;
    SECTION("0.5 inches is 36 stored units and back") {
        REQUIRE(inchesToStored(0.5) == 36);
        REQUIRE(storedToInchesString(36) == "0.5");
        REQUIRE(storedToInchesString(0) == "0");
        REQUIRE(storedToInchesString(72) == "1");
        REQUIRE(storedToInchesString(108) == "1.5");
    }
    SECTION("first-line indent keeps its sign") {
        REQUIRE(inchesToStoredSigned(-0.25) == -18);
        REQUIRE(storedToInchesString(-18) == "-0.25");
        REQUIRE(inchesToStored(-0.25) == 0);
    }
    SECTION("unparseable inches keep the seeded fallback") {
        REQUIRE(parseInches("", 0.5) == Approx(0.5));
        REQUIRE(parseInches("abc", 0.5) == Approx(0.5));
        REQUIRE(parseInches("0.25", 0.5) == Approx(0.25));
    }
    SECTION("spacing points clamp to 0..720, bad text keeps fallback") {
        REQUIRE(parsePoints("12", 0) == 12);
        REQUIRE(parsePoints("-3", 6) == 0);
        REQUIRE(parsePoints("9999", 6) == 720);
        REQUIRE(parsePoints("abc", 6) == 6);
        REQUIRE(parsePoints("", 6) == 6);
    }
    SECTION("tab stops clamp to the Tab Width range 1..16") {
        REQUIRE(parseTabStops("6", 4) == 6);
        REQUIRE(parseTabStops("0", 4) == 1);
        REQUIRE(parseTabStops("99", 4) == 16);
        REQUIRE(parseTabStops("abc", 4) == 4);
    }
    SECTION("alignment dropdown maps to TextAlignment and back") {
        REQUIRE(alignmentIndexFor(TextAlignment::Left) == 0);
        REQUIRE(alignmentIndexFor(TextAlignment::Center) == 1);
        REQUIRE(alignmentIndexFor(TextAlignment::Right) == 2);
        REQUIRE(alignmentIndexFor(TextAlignment::Justify) == 3);
        REQUIRE(alignmentForIndex(0) == TextAlignment::Left);
        REQUIRE(alignmentForIndex(1) == TextAlignment::Center);
        REQUIRE(alignmentForIndex(2) == TextAlignment::Right);
        REQUIRE(alignmentForIndex(3) == TextAlignment::Justify);
        REQUIRE(alignmentForIndex(7) == TextAlignment::Left);
        REQUIRE(std::string(kAlignmentNames[1]) == "Centered");
        REQUIRE(std::string(kAlignmentNames[3]) == "Justified");
    }
    SECTION("special indent maps from a stored first-line indent") {
        REQUIRE(specialIndexFor(0) == kSpecialNone);
        REQUIRE(specialIndexFor(36) == kSpecialFirstLine);
        REQUIRE(specialIndexFor(-36) == kSpecialHanging);
        REQUIRE(std::string(kSpecialNames[2]) == "Hanging");
    }
    SECTION("(Special, By) drives firstLineIndent, None ignores By") {
        REQUIRE(firstLineIndentFor(kSpecialNone, 0.5) == 0);
        REQUIRE(firstLineIndentFor(kSpecialFirstLine, 0.5) == 36);
        REQUIRE(firstLineIndentFor(kSpecialHanging, 0.5) == -36);
        REQUIRE(firstLineIndentFor(kSpecialFirstLine, -0.5) == 0);
        REQUIRE(firstLineIndentFor(kSpecialHanging, 0.0) == 0);
        REQUIRE(firstLineIndentFor(9, 0.5) == 0);
    }
    SECTION("line spacing maps to the nearest dropdown entry") {
        REQUIRE(lineSpacingIndexFor(1.0f) == 0);
        REQUIRE(lineSpacingIndexFor(1.5f) == 1);
        REQUIRE(lineSpacingIndexFor(2.0f) == 2);
        REQUIRE(lineSpacingIndexFor(1.25f) == 0);
        REQUIRE(lineSpacingIndexFor(1.75f) == 1);
        REQUIRE(lineSpacingForIndex(0) == 1.0f);
        REQUIRE(lineSpacingForIndex(1) == 1.5f);
        REQUIRE(lineSpacingForIndex(2) == 2.0f);
        REQUIRE(lineSpacingForIndex(7) == 1.0f);
    }
}
