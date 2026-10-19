#include "../src/editor/columns_math.h"
#include "../src/editor/document_io.h"

#include <filesystem>

#include "catch2/catch.hpp"

TEST_CASE("Columns assignment math", "[columns]") {
    using namespace columns_math;
    // 2 columns, 3 lines per column: page 0 holds lines 0-5.
    SECTION("first column fills top-to-bottom") {
        auto a = columnAssignment(0, 3, 2);
        REQUIRE(a.page == 0);
        REQUIRE(a.column == 0);
        REQUIRE(a.rowInColumn == 0);
        a = columnAssignment(2, 3, 2);
        REQUIRE(a.column == 0);
        REQUIRE(a.rowInColumn == 2);
    }
    SECTION("second column starts after the first is full") {
        auto a = columnAssignment(3, 3, 2);
        REQUIRE(a.page == 0);
        REQUIRE(a.column == 1);
        REQUIRE(a.rowInColumn == 0);
        a = columnAssignment(5, 3, 2);
        REQUIRE(a.column == 1);
        REQUIRE(a.rowInColumn == 2);
    }
    SECTION("next page starts after all columns") {
        auto a = columnAssignment(6, 3, 2);
        REQUIRE(a.page == 1);
        REQUIRE(a.column == 0);
        REQUIRE(a.rowInColumn == 0);
        a = columnAssignment(11, 3, 2);
        REQUIRE(a.page == 1);
        REQUIRE(a.column == 1);
        REQUIRE(a.rowInColumn == 2);
    }
    SECTION("single column paginates by lines per column") {
        auto a = columnAssignment(7, 5, 1);
        REQUIRE(a.page == 1);
        REQUIRE(a.column == 0);
        REQUIRE(a.rowInColumn == 2);
    }
    SECTION("degenerate inputs are safe") {
        auto a = columnAssignment(4, 0, 0);
        REQUIRE(a.page == 4);
        REQUIRE(a.column == 0);
        REQUIRE(a.rowInColumn == 0);
    }
}

TEST_CASE("Columns widths and offsets", "[columns]") {
    using namespace columns_math;
    SECTION("width splits the text width minus gutters") {
        REQUIRE(columnWidth(600.0f, 1, 36.0f) == 600.0f);
        REQUIRE(columnWidth(600.0f, 2, 36.0f) == 282.0f);
        REQUIRE(columnWidth(600.0f, 3, 36.0f) == 176.0f);
        REQUIRE(columnWidth(600.0f, 4, 36.0f) == 123.0f);
    }
    SECTION("offsets step by width + spacing") {
        REQUIRE(columnXOffset(0, 282.0f, 36.0f) == 0.0f);
        REQUIRE(columnXOffset(1, 282.0f, 36.0f) == 318.0f);
        REQUIRE(columnXOffset(2, 282.0f, 36.0f) == 636.0f);
    }
    SECTION("lines per column from the page text height") {
        // Letter (792pt) minus 1in top/bottom margins = 648pt of
        // text; at 0.5 display scale that is 324px, / 13px lines.
        REQUIRE(linesPerColumnForPage(792.0f, 72.0f, 0.5f, 13) == 24);
        REQUIRE(linesPerColumnForPage(792.0f, 72.0f, 0.5f, 0) == 1);
    }
    SECTION("inches formatting trims zeros") {
        REQUIRE(pointsToInchesString(36.0f) == "0.5");
        REQUIRE(pointsToInchesString(72.0f) == "1");
        REQUIRE(pointsToInchesString(18.0f) == "0.25");
        REQUIRE(pointsToInchesString(0.0f) == "0");
    }
}

TEST_CASE("Columns settings survive a save/load roundtrip", "[columns]") {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_columns_test";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "cols.wpdoc").string();

    TextBuffer original;
    original.setText("one\ntwo");
    DocumentSettings settings;
    settings.pageSettings.columnCount = 3;
    settings.pageSettings.columnSpacing = 18.0f;
    REQUIRE(saveDocumentEx(original, settings, path).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    REQUIRE(loadedSettings.pageSettings.columnCount == 3);
    REQUIRE(loadedSettings.pageSettings.columnSpacing == 18.0f);
}
