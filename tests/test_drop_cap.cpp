#include <filesystem>

#include "../src/editor/document_io.h"
#include "../src/editor/drop_cap.h"
#include "../src/editor/text_buffer.h"
#include "../src/editor/text_layout.h"
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

    SECTION("wrapIndentApplies: only indices [1, N) of the same paragraph") {
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(3, 0));
        REQUIRE(drop_cap::wrapIndentApplies(3, 1));
        REQUIRE(drop_cap::wrapIndentApplies(3, 2));
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(3, 3));
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(3, 4));
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(0, 1));
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(2, 0));
        REQUIRE(drop_cap::wrapIndentApplies(2, 1));
        REQUIRE_FALSE(drop_cap::wrapIndentApplies(2, 2));
    }

    SECTION("coveringDropCapParagraph: only the cap paragraph itself") {
        using Row = drop_cap::RowInfo;
        std::vector<Row> rows = {{true, 3}, {}, {true, 2}, {}};
        // The cap paragraph's own wrapped lines 1-2 indent.
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 0, 1) == 0);
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 0, 2) == 0);
        // Its first line carries the glyph (no indent); line 3 is
        // past the span.
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 0, 0) == -1);
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 0, 3) == -1);
        // Paragraphs 1 and 3 are NEVER covered by paragraph 0's
        // cap, however close they sit after it (the scope leak:
        // the old covering-row scan indented them).
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 1, 1) == -1);
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 3, 1) == -1);
        // Paragraph 2 carries its own cap: its wrapped line 1
        // indents, line 2 does not (span 2).
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 2, 1) == 2);
        REQUIRE(drop_cap::coveringDropCapParagraph(rows, 2, 2) == -1);
    }

    SECTION("3 paragraphs, cap on p1: p2/p3 unindented, p1 lines 2..N indented") {
        // p1 is long enough to wrap into 4 lines at width 10;
        // p2/p3 are short (one wrapped line each).
        TextBuffer buffer;
        buffer.setText("First paragraph wraps a lot here\nSecond\nThird");
        ParagraphFormat fmt = buffer.lineParagraphFormat(0);
        fmt.dropCapLines = 3;
        buffer.setLineParagraphFormat(0, fmt);
        auto wrapped = layoutWrappedLines(buffer, 10);
        // Wrapped index within each paragraph, in layout order.
        std::vector<std::size_t> wrappedIndex(buffer.lineCount(), 0);
        for (const auto& line : wrapped) {
            std::size_t para = line.source_row;
            std::size_t index = wrappedIndex[para]++;
            std::vector<drop_cap::RowInfo> rows;
            for (std::size_t r = 0; r < buffer.lineCount(); ++r) {
                rows.push_back(drop_cap::RowInfo{
                    buffer.lineDropCapLines(r) >= drop_cap::kMinLines,
                    buffer.lineDropCapLines(r)});
            }
            bool indented = drop_cap::coveringDropCapParagraph(
                                rows, para, index) >= 0;
            if (para == 0) {
                // p1: lines 2..3 (indices 1,2) indented, line 4 not.
                REQUIRE(indented == (index == 1 || index == 2));
            } else {
                // p2/p3: first (only) line at the normal margin.
                REQUIRE(index == 0);
                REQUIRE_FALSE(indented);
            }
        }
        // The layout really did wrap p1 into 4 lines.
        REQUIRE(wrapped.size() == 6);
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
