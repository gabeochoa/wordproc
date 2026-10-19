#include <filesystem>

#include "../src/editor/document_io.h"
#include "../src/editor/text_buffer.h"
#include "../src/editor/toc.h"
#include "catch2/catch.hpp"

namespace {

void styleRow(TextBuffer& buffer, std::size_t row, ParagraphStyle style) {
    buffer.setCaret({row, 0});
    buffer.setCurrentParagraphStyle(style);
}

TextBuffer headedDoc() {
    TextBuffer buffer;
    buffer.setText("Doc Title\nIntro\nBody\nChapter Two\nMore");
    styleRow(buffer, 0, ParagraphStyle::Title);
    styleRow(buffer, 1, ParagraphStyle::Heading1);
    styleRow(buffer, 3, ParagraphStyle::Heading2);
    return buffer;
}

std::size_t countOccurrences(const std::string& text,
                             const std::string& needle) {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

}  // namespace

TEST_CASE("TOC entry formatting", "[toc]") {
    SECTION("pageForLine is 1-based rows-per-page") {
        REQUIRE(toc::pageForLine(0, 50) == 1);
        REQUIRE(toc::pageForLine(49, 50) == 1);
        REQUIRE(toc::pageForLine(50, 50) == 2);
        REQUIRE(toc::pageForLine(0, 0) == 1);
    }

    SECTION("entry line: indent, dot leader to column 44, page") {
        std::string line = toc::entryLine(1, "Intro", 3);
        // "  Intro" is 7 chars; 44 - 7 = 37 dots; then " 3".
        REQUIRE(line.rfind("  Intro", 0) == 0);
        REQUIRE(line.size() == 7 + 37 + 2);
        REQUIRE(line.substr(line.size() - 2) == " 3");
        REQUIRE(line.substr(7, 37) == std::string(37, '.'));
    }

    SECTION("entry line: level 0 has no indent, long text keeps 3 dots") {
        std::string longText(50, 'x');
        std::string line = toc::entryLine(0, longText, 1);
        REQUIRE(line.rfind(longText, 0) == 0);
        REQUIRE(line.substr(50, 3) == "...");
        REQUIRE(line.substr(line.size() - 2) == " 1");
    }

    SECTION("isEntryLine accepts generated lines only") {
        REQUIRE(toc::isEntryLine(toc::entryLine(2, "Chapter Two", 4)));
        REQUIRE_FALSE(toc::isEntryLine("Hello"));
        REQUIRE_FALSE(toc::isEntryLine("Title...."));
        REQUIRE_FALSE(toc::isEntryLine("A.. 1"));
        REQUIRE_FALSE(toc::isEntryLine(""));
    }

    SECTION("block text is title plus one line per entry") {
        std::string block = toc::blockText(
            {{"A", 0, 0, 1}, {"B", 1, 1, 1}});
        REQUIRE(block ==
                "Table of Contents\n" + toc::entryLine(0, "A", 1) + "\n" +
                    toc::entryLine(1, "B", 1) + "\n");
    }
}

TEST_CASE("TOC insert marks a generated region", "[toc]") {
    TextBuffer buffer = headedDoc();
    buffer.setCaret({0, 0});

    SECTION("insert at top: text, mark, entries, shifted page numbers") {
        REQUIRE(buffer.insertTableOfContents(2));
        REQUIRE(buffer.hasToc());
        REQUIRE(buffer.tocStartOffset() == 0);
        REQUIRE(buffer.tocEntryCount() == 3);
        // Block is 1 title + 3 entries = 4 lines, so every heading
        // shifts 4 rows: Title 0 -> 4 (page 3), Intro 1 -> 5
        // (page 3), Chapter Two 3 -> 7 (page 4).
        std::string expected =
            "Table of Contents\n" + toc::entryLine(0, "Doc Title", 3) +
            "\n" + toc::entryLine(1, "Intro", 3) + "\n" +
            toc::entryLine(2, "Chapter Two", 4) + "\n";
        REQUIRE(buffer.getText().substr(0, expected.size()) == expected);
        REQUIRE(buffer.tocEndOffset() == expected.size());
        REQUIRE(buffer.getText().substr(expected.size()) ==
                "Doc Title\nIntro\nBody\nChapter Two\nMore");
        // Generated lines are Normal: the TOC title must not
        // inherit the abutting heading's Title style, or the
        // outline would list the TOC itself.
        REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Normal);
        REQUIRE(buffer.getOutline().size() == 3);
    }

    SECTION("no headings: insert is a no-op with no mark") {
        TextBuffer plain;
        plain.setText("Just body\nMore body");
        plain.setCaret({0, 0});
        REQUIRE_FALSE(plain.insertTableOfContents());
        REQUIRE_FALSE(plain.hasToc());
        REQUIRE(plain.getText() == "Just body\nMore body");
    }

    SECTION("Heading 4-6 and Subtitle are not entries") {
        TextBuffer mixed;
        mixed.setText("T\nH4\nSub\nH3");
        styleRow(mixed, 0, ParagraphStyle::Title);
        styleRow(mixed, 1, ParagraphStyle::Heading4);
        styleRow(mixed, 2, ParagraphStyle::Subtitle);
        styleRow(mixed, 3, ParagraphStyle::Heading3);
        mixed.setCaret({0, 0});
        REQUIRE(mixed.insertTableOfContents());
        REQUIRE(mixed.tocEntryCount() == 2);
        // H4 stays body text below the TOC; it is not an entry.
        REQUIRE(mixed.getText().find("H4") != std::string::npos);
        // The TOC entries are T and H3 only.
        REQUIRE(countOccurrences(mixed.getText(), "Table of Contents") ==
                1);
    }
}

TEST_CASE("TOC update replaces the region exactly", "[toc]") {
    TextBuffer buffer = headedDoc();
    buffer.setCaret({0, 0});
    REQUIRE(buffer.insertTableOfContents(2));

    SECTION("adding a heading then updating: one block, new entry") {
        buffer.moveToDocumentEnd();
        buffer.insertText("\nNew Section");
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        REQUIRE(buffer.updateTableOfContents(2));
        REQUIRE(countOccurrences(buffer.getText(),
                                 "Table of Contents") == 1);
        REQUIRE(buffer.tocEntryCount() == 4);
        REQUIRE(buffer.getText().find("New Section") !=
                std::string::npos);
    }

    SECTION("inserting again replaces instead of duplicating") {
        buffer.setCaret({0, 0});
        REQUIRE(buffer.insertTableOfContents(2));
        REQUIRE(countOccurrences(buffer.getText(),
                                 "Table of Contents") == 1);
        REQUIRE(buffer.tocEntryCount() == 3);
    }

    SECTION("hand edits inside the region are lost on update") {
        std::size_t at = buffer.getText().find("Intro");
        REQUIRE(at != std::string::npos);
        buffer.replaceRange(at, 5, "XXXXX");
        REQUIRE(buffer.getText().find("XXXXX") != std::string::npos);
        REQUIRE(buffer.updateTableOfContents(2));
        REQUIRE(buffer.getText().find("XXXXX") == std::string::npos);
        REQUIRE(buffer.getText().find("Intro") != std::string::npos);
    }

    SECTION("caret after the block shifts by the length delta") {
        buffer.moveToDocumentEnd();
        buffer.insertText("\nNew Section");
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        std::size_t caretAtEnd = buffer.caretOffset();
        std::size_t oldEnd = buffer.tocEndOffset();
        REQUIRE(buffer.updateTableOfContents(2));
        // One entry added: the block grows by one entry line.
        REQUIRE(buffer.tocEntryCount() == 4);
        REQUIRE(buffer.caretOffset() ==
                caretAtEnd + (buffer.tocEndOffset() - oldEnd));
    }

    SECTION("caret inside the block lands at the block end") {
        buffer.moveToDocumentEnd();
        buffer.insertText("\nNew Section");
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        buffer.setCaret({1, 0});  // First entry line
        REQUIRE(buffer.updateTableOfContents(2));
        REQUIRE(buffer.tocEntryCount() == 4);
        REQUIRE(buffer.caretOffset() == buffer.tocEndOffset());
    }

    SECTION("update with no headings erases the block") {
        // Restyle every heading back to Normal.
        for (std::size_t row = 0; row < buffer.lineCount(); ++row) {
            styleRow(buffer, row, ParagraphStyle::Normal);
        }
        REQUIRE_FALSE(buffer.updateTableOfContents(2));
        REQUIRE_FALSE(buffer.hasToc());
        REQUIRE(buffer.getText().find("Table of Contents") ==
                std::string::npos);
    }
}

TEST_CASE("TOC mark shifts on edits before it", "[toc]") {
    TextBuffer buffer = headedDoc();
    buffer.moveToDocumentEnd();
    REQUIRE(buffer.insertTableOfContents(50));
    std::size_t oldStart = buffer.tocStartOffset();
    std::size_t oldEnd = buffer.tocEndOffset();

    SECTION("typing a line before the TOC shifts the mark") {
        buffer.setCaret({0, 0});
        buffer.insertText("Preamble\n");
        REQUIRE(buffer.hasToc());
        REQUIRE(buffer.tocStartOffset() == oldStart + 9);
        REQUIRE(buffer.tocEndOffset() == oldEnd + 9);
        // And an update still regenerates the same block.
        REQUIRE(buffer.updateTableOfContents(50));
        REQUIRE(buffer.tocEntryCount() == 3);
        REQUIRE(countOccurrences(buffer.getText(),
                                 "Table of Contents") == 1);
    }

    SECTION("deleting a line before the TOC shifts the mark back") {
        buffer.setCaret({0, 0});
        buffer.insertText("Preamble\n");
        // Delete the preamble line via selection: select row 0.
        buffer.setCaret({0, 0});
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({1, 0});
        buffer.updateSelectionToCaret();
        REQUIRE(buffer.deleteSelection());
        REQUIRE(buffer.tocStartOffset() == oldStart);
        REQUIRE(buffer.tocEndOffset() == oldEnd);
    }
}

TEST_CASE("TOC undo restores text and mark", "[toc]") {
    TextBuffer buffer = headedDoc();
    std::string before = buffer.getText();

    SECTION("undo of insert clears the mark") {
        buffer.setCaret({0, 0});
        REQUIRE(buffer.insertTableOfContents(50));
        REQUIRE(buffer.hasToc());
        buffer.undo();
        REQUIRE_FALSE(buffer.hasToc());
        REQUIRE(buffer.getText() == before);
        // Undo must not steal the boundary heading's style.
        REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Title);
    }

    SECTION("undo of update restores the old block") {
        buffer.setCaret({0, 0});
        REQUIRE(buffer.insertTableOfContents(50));
        std::size_t oldEnd = buffer.tocEndOffset();
        buffer.moveToDocumentEnd();
        buffer.insertText("\nNew Section");
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        REQUIRE(buffer.updateTableOfContents(50));
        REQUIRE(buffer.tocEntryCount() == 4);
        buffer.undo();
        REQUIRE(buffer.hasToc());
        REQUIRE(buffer.tocEntryCount() == 3);
        REQUIRE(buffer.tocEndOffset() == oldEnd);
        // Undo must not steal the first heading's style, or a
        // second update would drop its entry. Outline is 4: the
        // added "New Section" heading stays in the document; only
        // the TOC block reverts.
        REQUIRE(buffer.getOutline().size() == 4);
        REQUIRE(buffer.updateTableOfContents(50));
        REQUIRE(buffer.tocEntryCount() == 4);
    }
}

TEST_CASE("TOC mark persists through save/load", "[toc][document_io]") {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_test_toc";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "toc.wpdoc").string();

    TextBuffer buffer = headedDoc();
    buffer.setCaret({0, 0});
    REQUIRE(buffer.insertTableOfContents(50));
    DocumentSettings settings;
    REQUIRE(saveDocumentEx(buffer, settings, path).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    REQUIRE(loaded.hasToc());
    REQUIRE(loaded.tocEntryCount() == 3);
    REQUIRE(loaded.getText() == buffer.getText());
    // A loaded TOC updates in place.
    loaded.moveToDocumentEnd();
    loaded.insertText("\nNew Section");
    loaded.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    REQUIRE(loaded.updateTableOfContents(50));
    REQUIRE(loaded.tocEntryCount() == 4);
    REQUIRE(countOccurrences(loaded.getText(), "Table of Contents") ==
            1);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
