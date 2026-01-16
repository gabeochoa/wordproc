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
        // Create a selection by using shift movement
        buffer.setCaret({0, 0});
        buffer.startSelection();
        buffer.setCaret({3, 5});
        buffer.updateSelection();
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
