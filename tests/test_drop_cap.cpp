#include <filesystem>

#include "../src/editor/document_io.h"
#include "../src/editor/drop_cap.h"
#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

TEST_CASE("Drop cap helpers", "[drop_cap]") {
    SECTION("clampLines: 0/1 stay off, 2-4 pass, above clamps") {
        REQUIRE(drop_cap::clampLines(0) == 0);
        REQUIRE(drop_cap::clampLines(1) == 0);
        REQUIRE(drop_cap::clampLines(-3) == 0);
        REQUIRE(drop_cap::clampLines(2) == 2);
        REQUIRE(drop_cap::clampLines(3) == 3);
        REQUIRE(drop_cap::clampLines(4) == 4);
        REQUIRE(drop_cap::clampLines(5) == 4);
        REQUIRE_FALSE(drop_cap::isEnabled(0));
        REQUIRE(drop_cap::isEnabled(2));
    }

    SECTION("coveringDropCapRow: nearest span that reaches the row") {
        using Row = drop_cap::RowInfo;
        std::vector<Row> rows = {{true, 3}, {}, {true, 2}, {}};
        REQUIRE(drop_cap::coveringDropCapRow(rows, 1) == 0);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 2) == 0);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 3) == 2);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 0) == -1);
    }

    SECTION("coveringDropCapRow: a short span does not cover") {
        using Row = drop_cap::RowInfo;
        std::vector<Row> rows = {{true, 2}, {}, {}, {}};
        REQUIRE(drop_cap::coveringDropCapRow(rows, 1) == 0);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 2) == -1);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 3) == -1);
    }

    SECTION("coveringDropCapRow: nearer span wins over an earlier one") {
        using Row = drop_cap::RowInfo;
        std::vector<Row> rows = {{true, 4}, {true, 2}, {}};
        REQUIRE(drop_cap::coveringDropCapRow(rows, 1) == 0);
        REQUIRE(drop_cap::coveringDropCapRow(rows, 2) == 1);
    }
}

TEST_CASE("Drop cap paragraph format", "[drop_cap]") {
    TextBuffer buffer;
    buffer.setText("First\nSecond\nThird");

    SECTION("format carries the span; off is 0") {
        REQUIRE(buffer.lineDropCapLines(0) == 0);
        ParagraphFormat fmt = buffer.lineParagraphFormat(0);
        fmt.dropCapLines = 3;
        buffer.setLineParagraphFormat(0, fmt);
        REQUIRE(buffer.lineDropCapLines(0) == 3);
        REQUIRE(buffer.currentLineHasDropCap() == false);
        buffer.setCaret({0, 0});
        REQUIRE(buffer.currentDropCapLines() == 3);
        REQUIRE(buffer.currentLineHasDropCap());
        // Neighbor untouched.
        REQUIRE(buffer.lineDropCapLines(1) == 0);
    }

    SECTION("one undo step restores the old format") {
        buffer.setCaret({0, 0});
        ParagraphFormat old = buffer.lineParagraphFormat(0);
        ParagraphFormat next = old;
        next.dropCapLines = 3;
        buffer.setLineParagraphFormat(0, next);
        buffer.commitParagraphFormat({{0, old, next}});
        REQUIRE(buffer.lineDropCapLines(0) == 3);
        buffer.undo();
        REQUIRE(buffer.lineDropCapLines(0) == 0);
        buffer.redo();
        REQUIRE(buffer.lineDropCapLines(0) == 3);
    }

    SECTION("setCurrentLineDropCap clamps the span") {
        buffer.setCaret({1, 0});
        buffer.setCurrentLineDropCap(true, 9);
        REQUIRE(buffer.lineDropCapLines(1) == 4);
        buffer.setCurrentLineDropCap(false);
        REQUIRE(buffer.lineDropCapLines(1) == 0);
    }
}

TEST_CASE("Drop cap persists through save/load", "[drop_cap][document_io]") {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_test_dropcap";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "dropcap.wpdoc").string();

    TextBuffer buffer;
    buffer.setText("First\nSecond");
    ParagraphFormat fmt = buffer.lineParagraphFormat(0);
    fmt.dropCapLines = 3;
    buffer.setLineParagraphFormat(0, fmt);
    DocumentSettings settings;
    REQUIRE(saveDocumentEx(buffer, settings, path).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    REQUIRE(loaded.lineDropCapLines(0) == 3);
    REQUIRE(loaded.lineDropCapLines(1) == 0);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
