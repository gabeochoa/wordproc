#include "../src/editor/outline_view.h"
#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

TEST_CASE("Outline extraction", "[text_buffer][outline]") {
    TextBuffer buffer;
    
    SECTION("empty document has empty outline") {
        auto outline = buffer.getOutline();
        REQUIRE(outline.empty());
    }
    
    SECTION("document with no headings has empty outline") {
        buffer.setText("This is normal text.\nAnother normal line.");
        auto outline = buffer.getOutline();
        REQUIRE(outline.empty());
    }
    
    SECTION("extracts title from document") {
        buffer.setText("My Document Title");
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 1);
        REQUIRE(outline[0].text == "My Document Title");
        REQUIRE(outline[0].style == ParagraphStyle::Title);
        REQUIRE(outline[0].level == 0);
        REQUIRE(outline[0].lineNumber == 0);
    }
    
    SECTION("extracts multiple headings") {
        buffer.setText("Title\nIntroduction\nBackground\nMethods\nResults");
        
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        
        buffer.setCaret({1, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        
        buffer.setCaret({3, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        buffer.setCaret({4, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 5);
        
        REQUIRE(outline[0].style == ParagraphStyle::Title);
        REQUIRE(outline[1].style == ParagraphStyle::Heading1);
        REQUIRE(outline[2].style == ParagraphStyle::Heading2);
        REQUIRE(outline[3].style == ParagraphStyle::Heading1);
        REQUIRE(outline[4].style == ParagraphStyle::Heading1);
    }
    
    SECTION("outline levels are correct") {
        buffer.setText("H1\nH2\nH3\nH4\nH5\nH6");
        
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        buffer.setCaret({1, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
        buffer.setCaret({3, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
        buffer.setCaret({4, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
        buffer.setCaret({5, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
        
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 6);
        
        REQUIRE(outline[0].level == 1);  // H1
        REQUIRE(outline[1].level == 2);  // H2
        REQUIRE(outline[2].level == 3);  // H3
        REQUIRE(outline[3].level == 4);  // H4
        REQUIRE(outline[4].level == 5);  // H5
        REQUIRE(outline[5].level == 6);  // H6
    }
    
    SECTION("long headings are truncated") {
        std::string longText = "This is a very long heading that should be truncated because it exceeds sixty characters";
        buffer.setText(longText);
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 1);
        REQUIRE(outline[0].text.length() <= 60);
        REQUIRE(outline[0].text.find("...") != std::string::npos);
    }
    
    SECTION("skips normal paragraphs") {
        buffer.setText("Title\nNormal text\nHeading 1\nMore normal text\nHeading 2");
        
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        // Line 1 stays Normal
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        // Line 3 stays Normal
        buffer.setCaret({4, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 3);  // Title + 2 headings, skips normal text
    }
}

TEST_CASE("Outline navigation", "[text_buffer][outline]") {
    TextBuffer buffer;
    buffer.setText("Title\nIntro\nSection 1\nContent\nSection 2");
    
    buffer.setCaret({0, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
    buffer.setCaret({1, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
    buffer.setCaret({2, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    buffer.setCaret({4, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    
    SECTION("goToOutlineEntry navigates to line") {
        buffer.setCaret({3, 0});  // Start somewhere else
        REQUIRE(buffer.caret().row == 3);
        
        REQUIRE(buffer.goToOutlineEntry(2));
        REQUIRE(buffer.caret().row == 2);
        REQUIRE(buffer.caret().column == 0);
    }
    
    SECTION("goToOutlineEntry clears selection") {
        // Create a selection using setSelectionAnchor and updateSelectionToCaret
        buffer.setCaret({0, 0});
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({3, 5});
        buffer.updateSelectionToCaret();
        REQUIRE(buffer.hasSelection());
        
        buffer.goToOutlineEntry(4);
        REQUIRE_FALSE(buffer.hasSelection());
    }
    
    SECTION("goToOutlineEntry returns false for invalid line") {
        REQUIRE_FALSE(buffer.goToOutlineEntry(100));
    }
    
    SECTION("navigate through outline entries") {
        auto outline = buffer.getOutline();
        REQUIRE(outline.size() == 4);
        
        for (const auto& entry : outline) {
            REQUIRE(buffer.goToOutlineEntry(entry.lineNumber));
            REQUIRE(buffer.caret().row == entry.lineNumber);
        }
    }
}

TEST_CASE("Table of contents generation", "[text_buffer][outline][toc]") {
    TextBuffer buffer;
    
    SECTION("empty document generates empty TOC") {
        std::string toc = buffer.generateTableOfContents();
        REQUIRE(toc.empty());
    }
    
    SECTION("document without headings generates empty TOC") {
        buffer.setText("Normal text\nMore normal text");
        std::string toc = buffer.generateTableOfContents();
        REQUIRE(toc.empty());
    }
    
    SECTION("generates TOC from headings") {
        buffer.setText("Title\nIntro\nChapter 1\nSection 1.1\nChapter 2");
        
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        buffer.setCaret({1, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        buffer.setCaret({3, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        buffer.setCaret({4, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        std::string toc = buffer.generateTableOfContents();
        REQUIRE(!toc.empty());
        REQUIRE(toc.find("Table of Contents") != std::string::npos);
        REQUIRE(toc.find("Title") != std::string::npos);
        REQUIRE(toc.find("Intro") != std::string::npos);
        REQUIRE(toc.find("Chapter 1") != std::string::npos);
        REQUIRE(toc.find("Section 1.1") != std::string::npos);
        REQUIRE(toc.find("Chapter 2") != std::string::npos);
    }
    
    SECTION("insertTableOfContents adds TOC to document") {
        buffer.setText("Title\nContent\nHeading");
        
        buffer.setCaret({0, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        buffer.setCaret({2, 0});
        buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        
        // Move to end and insert TOC
        buffer.setCaret({2, buffer.lines()[2].length()});
        buffer.insertChar('\n');
        buffer.insertTableOfContents();
        
        std::string text = buffer.getText();
        REQUIRE(text.find("Table of Contents") != std::string::npos);
    }
}

TEST_CASE("Outline Promote / Demote", "[outline][outline_view]") {
    using namespace outline_view;
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Heading2) ==
            ParagraphStyle::Heading1);
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Heading6) ==
            ParagraphStyle::Heading5);
    // Clamps at the top: Heading 1 stays Heading 1.
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Heading1) ==
            ParagraphStyle::Heading1);
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Subtitle) ==
            ParagraphStyle::Title);
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Title) ==
            ParagraphStyle::Title);
    REQUIRE(demoteParagraphStyle(ParagraphStyle::Heading1) ==
            ParagraphStyle::Heading2);
    REQUIRE(demoteParagraphStyle(ParagraphStyle::Heading5) ==
            ParagraphStyle::Heading6);
    // Clamps at Heading 6.
    REQUIRE(demoteParagraphStyle(ParagraphStyle::Heading6) ==
            ParagraphStyle::Heading6);
    REQUIRE(demoteParagraphStyle(ParagraphStyle::Title) ==
            ParagraphStyle::Subtitle);
    // Normal is not an outline row: unchanged.
    REQUIRE(promoteParagraphStyle(ParagraphStyle::Normal) ==
            ParagraphStyle::Normal);
    REQUIRE(demoteParagraphStyle(ParagraphStyle::Normal) ==
            ParagraphStyle::Normal);
}

TEST_CASE("Outline blocks and sibling blocks", "[outline][outline_view]") {
    using namespace outline_view;
    // H1, H2, body, H1, H2, body, H1
    std::vector<ParagraphStyle> styles = {
        ParagraphStyle::Heading1, ParagraphStyle::Heading2,
        ParagraphStyle::Normal, ParagraphStyle::Heading1,
        ParagraphStyle::Heading2, ParagraphStyle::Normal,
        ParagraphStyle::Heading1};
    REQUIRE(outlineBlockEnd(styles, 0) == 3);
    REQUIRE(outlineBlockEnd(styles, 3) == 6);
    REQUIRE(outlineBlockEnd(styles, 6) == 7);
    REQUIRE(outlineBlockEnd(styles, 1) == 3);
    REQUIRE(outlineHasSubordinateHeadings(styles, 0));
    REQUIRE_FALSE(outlineHasSubordinateHeadings(styles, 1));
    REQUIRE_FALSE(outlineHasSubordinateHeadings(styles, 6));
    REQUIRE_FALSE(outlineHasSubordinateHeadings(styles, 2));

    auto prev = outlinePrevSiblingBlock(styles, 3);
    REQUIRE(prev.has_value());
    REQUIRE(prev->first == 0);
    REQUIRE(prev->second == 3);
    REQUIRE_FALSE(outlinePrevSiblingBlock(styles, 0).has_value());
    // The second H1's previous sibling is the first H1, and the
    // H2 at 1 has no previous same-level sibling (its parent is
    // the H1 at 0, not a sibling to move past).
    REQUIRE_FALSE(outlinePrevSiblingBlock(styles, 1).has_value());
    auto next = outlineNextSiblingBlock(styles, 3);
    REQUIRE(next.has_value());
    REQUIRE(next->first == 6);
    REQUIRE(next->second == 7);
    REQUIRE_FALSE(outlineNextSiblingBlock(styles, 6).has_value());
    REQUIRE_FALSE(outlineNextSiblingBlock(styles, 1).has_value());
    // H2 at 4: next sibling scanning stops at the shallower H1.
    REQUIRE_FALSE(outlineNextSiblingBlock(styles, 4).has_value());
}

TEST_CASE("Outline filtered visible rows", "[outline][outline_view]") {
    using namespace outline_view;
    // H1, H2, body, H1, H2, H3, H1
    std::vector<ParagraphStyle> styles = {
        ParagraphStyle::Heading1, ParagraphStyle::Heading2,
        ParagraphStyle::Normal, ParagraphStyle::Heading1,
        ParagraphStyle::Heading2, ParagraphStyle::Heading3,
        ParagraphStyle::Heading1};
    std::unordered_set<std::size_t> none;
    auto all = outlineVisibleRowsFiltered(styles, 0, none);
    REQUIRE(all.size() == 6);

    // Show Level 1 hides every heading deeper than 1.
    auto level1 = outlineVisibleRowsFiltered(styles, 1, none);
    REQUIRE(level1.size() == 3);
    REQUIRE(level1[0] == 0);
    REQUIRE(level1[1] == 3);
    REQUIRE(level1[2] == 6);
    // Show Level 2 keeps H2 but hides H3.
    auto level2 = outlineVisibleRowsFiltered(styles, 2, none);
    REQUIRE(level2.size() == 5);

    // Collapsing the first H1 hides its H2 subordinate, but the
    // first H1 itself stays visible.
    std::unordered_set<std::size_t> collapsed = {0};
    auto collapsedRows = outlineVisibleRowsFiltered(styles, 0, collapsed);
    REQUIRE(collapsedRows.size() == 5);
    REQUIRE(collapsedRows[0] == 0);
    REQUIRE(collapsedRows[1] == 3);
    // Collapsing the second H1 hides its H2 and H3.
    collapsed = {3};
    collapsedRows = outlineVisibleRowsFiltered(styles, 0, collapsed);
    REQUIRE(collapsedRows.size() == 4);
    REQUIRE(collapsedRows[0] == 0);
    REQUIRE(collapsedRows[1] == 1);
    REQUIRE(collapsedRows[2] == 3);
    REQUIRE(collapsedRows[3] == 6);
    // Collapsed + Show Level combine: Show Level 1 alone would
    // show 3 rows; collapsing a visible H1 cannot hide another
    // visible H1.
    collapsed = {0};
    auto both = outlineVisibleRowsFiltered(styles, 1, collapsed);
    REQUIRE(both.size() == 3);
}

TEST_CASE("Outline block swap moves text and spans, undoes in one step",
          "[outline][text_buffer]") {
    TextBuffer buffer;
    buffer.setText("First\nBody one\nSecond\nBody two\nThird");
    auto styleRow = [&](std::size_t row, ParagraphStyle style) {
        buffer.setCaret({row, 0});
        buffer.setCurrentParagraphStyle(style);
    };
    styleRow(0, ParagraphStyle::Heading1);
    styleRow(2, ParagraphStyle::Heading1);
    styleRow(4, ParagraphStyle::Heading1);
    // Second block [2, 4) swaps with the first block [0, 2).
    buffer.swapAdjacentLineRanges(0, 2, 4);
    buffer.commitSwapAdjacentLineRanges(0, 2, 4, 2, 0);
    REQUIRE(buffer.lineString(0) == "Second");
    REQUIRE(buffer.lineString(1) == "Body two");
    REQUIRE(buffer.lineString(2) == "First");
    REQUIRE(buffer.lineString(3) == "Body one");
    REQUIRE(buffer.lineString(4) == "Third");
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading1);
    REQUIRE(buffer.lineParagraphStyle(1) == ParagraphStyle::Normal);
    REQUIRE(buffer.lineParagraphStyle(2) == ParagraphStyle::Heading1);
    REQUIRE(buffer.lineParagraphStyle(3) == ParagraphStyle::Normal);
    REQUIRE(buffer.lineParagraphStyle(4) == ParagraphStyle::Heading1);
    // One undo step restores the original order AND styles.
    REQUIRE(buffer.canUndo());
    buffer.undo();
    REQUIRE(buffer.lineString(0) == "First");
    REQUIRE(buffer.lineString(1) == "Body one");
    REQUIRE(buffer.lineString(2) == "Second");
    REQUIRE(buffer.lineString(3) == "Body two");
    REQUIRE(buffer.lineString(4) == "Third");
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading1);
    REQUIRE(buffer.lineParagraphStyle(2) == ParagraphStyle::Heading1);
    // Redo re-applies the swap.
    REQUIRE(buffer.canRedo());
    buffer.redo();
    REQUIRE(buffer.lineString(0) == "Second");
    REQUIRE(buffer.lineString(2) == "First");
}

TEST_CASE("Outline block swap with unequal block sizes",
          "[outline][text_buffer]") {
    TextBuffer buffer;
    buffer.setText("A\nB\nC\nD");
    // Swap [0, 1) ("A") with [1, 3) ("B", "C"): the shifted
    // undo boundary (0 + 2 = 2) differs from the execute
    // boundary (1), so undo must use the shifted one.
    buffer.swapAdjacentLineRanges(0, 1, 3);
    buffer.commitSwapAdjacentLineRanges(0, 1, 3, 0, 2);
    REQUIRE(buffer.getText() == "B\nC\nA\nD");
    buffer.undo();
    REQUIRE(buffer.getText() == "A\nB\nC\nD");
    buffer.redo();
    REQUIRE(buffer.getText() == "B\nC\nA\nD");
}

TEST_CASE("Outline block swap at the end of the document",
          "[outline][text_buffer]") {
    TextBuffer buffer;
    buffer.setText("Alpha\nBeta\nGamma");
    buffer.setCaret({0, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    buffer.setCaret({1, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    buffer.setCaret({2, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    // Swap the last two single-line blocks (range ends at EOF:
    // the terminator must move, not duplicate).
    buffer.swapAdjacentLineRanges(1, 2, 3);
    buffer.commitSwapAdjacentLineRanges(1, 2, 3, 1, 2);
    REQUIRE(buffer.lineCount() == 3);
    REQUIRE(buffer.getText() == "Alpha\nGamma\nBeta");
    buffer.undo();
    REQUIRE(buffer.getText() == "Alpha\nBeta\nGamma");
}

TEST_CASE("Outline paragraph style commit is one undo step",
          "[outline][text_buffer]") {
    TextBuffer buffer;
    buffer.setText("Heading");
    buffer.setCaret({0, 0});
    buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
    buffer.clearHistory();
    buffer.setLineParagraphStyle(0, ParagraphStyle::Heading2);
    buffer.commitParagraphStyle(0, ParagraphStyle::Heading1,
                                ParagraphStyle::Heading2);
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading2);
    buffer.undo();
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading1);
    buffer.redo();
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Heading2);
}

TEST_CASE("Outline view visible-rows mapping", "[outline][outline_view]") {
    using namespace outline_view;
    std::vector<ParagraphStyle> styles = {
        ParagraphStyle::Title, ParagraphStyle::Normal,
        ParagraphStyle::Heading1, ParagraphStyle::Normal,
        ParagraphStyle::Heading2, ParagraphStyle::Subtitle,
        ParagraphStyle::Heading6, ParagraphStyle::Normal};

    SECTION("levels match getOutline") {
        REQUIRE(outlineLevel(ParagraphStyle::Title) == 0);
        REQUIRE(outlineLevel(ParagraphStyle::Subtitle) == 1);
        REQUIRE(outlineLevel(ParagraphStyle::Heading1) == 1);
        REQUIRE(outlineLevel(ParagraphStyle::Heading2) == 2);
        REQUIRE(outlineLevel(ParagraphStyle::Heading6) == 6);
        REQUIRE(outlineLevel(ParagraphStyle::Normal) == -1);
        REQUIRE_FALSE(isHeadingStyle(ParagraphStyle::Normal));
        REQUIRE(isHeadingStyle(ParagraphStyle::Heading3));
    }

    SECTION("visible rows skip body paragraphs") {
        auto rows = outlineVisibleRows(styles);
        REQUIRE(rows.size() == 5);
        REQUIRE(rows[0] == 0);  // Title
        REQUIRE(rows[1] == 2);  // Heading 1
        REQUIRE(rows[2] == 4);  // Heading 2
        REQUIRE(rows[3] == 5);  // Subtitle
        REQUIRE(rows[4] == 6);  // Heading 6
    }

    SECTION("visible index maps to the heading's real row, clamped") {
        auto rows = outlineVisibleRows(styles);
        REQUIRE(outlineRealRowForVisible(rows, 0, styles.size()) == 0);
        REQUIRE(outlineRealRowForVisible(rows, 2, styles.size()) == 4);
        // Past the end clamps to the last heading, never a body row.
        REQUIRE(outlineRealRowForVisible(rows, 99, styles.size()) == 6);
        // No headings: no such row.
        REQUIRE(outlineRealRowForVisible({}, 0, styles.size()) == styles.size());
    }

    SECTION("indent is (level - 1) * 24px") {
        REQUIRE(outlineIndentPx(ParagraphStyle::Title) == 0);
        REQUIRE(outlineIndentPx(ParagraphStyle::Heading1) == 0);
        REQUIRE(outlineIndentPx(ParagraphStyle::Heading2) == 24);
        REQUIRE(outlineIndentPx(ParagraphStyle::Heading3) == 48);
        REQUIRE(outlineIndentPx(ParagraphStyle::Heading6) == 120);
        REQUIRE(outlineIndentPx(ParagraphStyle::Normal) == 0);
    }
}
