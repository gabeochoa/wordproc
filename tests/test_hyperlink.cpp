#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

TEST_CASE("Hyperlink creation", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    
    SECTION("add hyperlink with selection") {
        // Select "World"
        buffer.setCaret({0, 6});
        buffer.setSelectionAnchor({0, 6});
        buffer.setCaret({0, 11});
        buffer.updateSelectionToCaret();
        
        REQUIRE(buffer.addHyperlink("https://example.com", "Example tooltip"));
        REQUIRE(buffer.hyperlinks().size() == 1);
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.url == "https://example.com");
        REQUIRE(link.tooltip == "Example tooltip");
        REQUIRE(link.startOffset == 6);
        REQUIRE(link.endOffset == 11);
    }
    
    SECTION("add hyperlink at specific offsets") {
        REQUIRE(buffer.addHyperlinkAt(0, 5, "https://hello.com"));
        REQUIRE(buffer.hyperlinks().size() == 1);
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 0);
        REQUIRE(link.endOffset == 5);
    }
    
    SECTION("cannot add hyperlink without selection") {
        buffer.clearSelection();
        REQUIRE_FALSE(buffer.addHyperlink("https://example.com"));
        REQUIRE(buffer.hyperlinks().empty());
    }
    
    SECTION("cannot add hyperlink with empty URL") {
        buffer.setCaret({0, 0});
        buffer.setSelectionAnchor({0, 0});
        buffer.setCaret({0, 5});
        buffer.updateSelectionToCaret();
        
        REQUIRE_FALSE(buffer.addHyperlink(""));
        REQUIRE(buffer.hyperlinks().empty());
    }
    
    SECTION("cannot add hyperlink with invalid offsets") {
        REQUIRE_FALSE(buffer.addHyperlinkAt(10, 5, "https://example.com"));  // end < start
        REQUIRE_FALSE(buffer.addHyperlinkAt(0, 100, "https://example.com")); // beyond buffer
        REQUIRE(buffer.hyperlinks().empty());
    }
}

TEST_CASE("Hyperlink lookup", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(0, 5, "https://hello.com");
    buffer.addHyperlinkAt(6, 11, "https://world.com");
    
    SECTION("hyperlinkAt finds correct link") {
        const auto* link1 = buffer.hyperlinkAt(2);
        REQUIRE(link1 != nullptr);
        REQUIRE(link1->url == "https://hello.com");
        
        const auto* link2 = buffer.hyperlinkAt(8);
        REQUIRE(link2 != nullptr);
        REQUIRE(link2->url == "https://world.com");
    }
    
    SECTION("hyperlinkAt returns nullptr for non-linked text") {
        // Position 5 is the space between words (not linked)
        const auto* link = buffer.hyperlinkAt(5);
        REQUIRE(link == nullptr);
    }
    
    SECTION("hyperlinkAtCaret returns correct link") {
        buffer.setCaret({0, 3});
        const auto* link = buffer.hyperlinkAtCaret();
        REQUIRE(link != nullptr);
        REQUIRE(link->url == "https://hello.com");
    }
    
    SECTION("hyperlinksInRange finds overlapping links") {
        auto links = buffer.hyperlinksInRange(0, 11);
        REQUIRE(links.size() == 2);
        
        links = buffer.hyperlinksInRange(3, 8);
        REQUIRE(links.size() == 2);
        
        links = buffer.hyperlinksInRange(0, 3);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0]->url == "https://hello.com");
    }
    
    SECTION("selectionHasHyperlink detects linked selection") {
        buffer.setCaret({0, 2});
        buffer.setSelectionAnchor({0, 2});
        buffer.setCaret({0, 4});
        buffer.updateSelectionToCaret();
        
        REQUIRE(buffer.selectionHasHyperlink());
        
        // Select just the space
        buffer.setCaret({0, 5});
        buffer.setSelectionAnchor({0, 5});
        buffer.setCaret({0, 6});
        buffer.updateSelectionToCaret();
        
        REQUIRE_FALSE(buffer.selectionHasHyperlink());
    }
}

TEST_CASE("Hyperlink editing", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(0, 5, "https://old.com", "Old tooltip");
    
    SECTION("edit hyperlink URL and tooltip") {
        REQUIRE(buffer.editHyperlink(2, "https://new.com", "New tooltip"));
        
        const auto* link = buffer.hyperlinkAt(2);
        REQUIRE(link != nullptr);
        REQUIRE(link->url == "https://new.com");
        REQUIRE(link->tooltip == "New tooltip");
    }
    
    SECTION("edit non-existent hyperlink returns false") {
        REQUIRE_FALSE(buffer.editHyperlink(8, "https://new.com"));
    }
}

TEST_CASE("Hyperlink removal", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(0, 5, "https://hello.com");
    buffer.addHyperlinkAt(6, 11, "https://world.com");
    
    SECTION("remove hyperlink keeps text") {
        std::string textBefore = buffer.getText();
        REQUIRE(buffer.removeHyperlink(2));
        REQUIRE(buffer.getText() == textBefore);  // Text unchanged
        REQUIRE(buffer.hyperlinks().size() == 1);
        REQUIRE(buffer.hyperlinkAt(2) == nullptr);
    }
    
    SECTION("remove non-existent hyperlink returns false") {
        REQUIRE_FALSE(buffer.removeHyperlink(5));  // Space between words
    }
}

TEST_CASE("Hyperlink offset adjustment on insert", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(6, 11, "https://world.com");  // "World"
    
    SECTION("insert before hyperlink shifts it") {
        buffer.setCaret({0, 0});
        buffer.insertText("Hi ");  // 3 chars
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 9);   // 6 + 3
        REQUIRE(link.endOffset == 14);    // 11 + 3
    }
    
    SECTION("insert within hyperlink expands it") {
        buffer.setCaret({0, 8});  // Inside "World"
        buffer.insertChar('x');
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 6);
        REQUIRE(link.endOffset == 12);  // 11 + 1
    }
    
    SECTION("insert after hyperlink leaves it unchanged") {
        buffer.setCaret({0, 11});  // After "World"
        buffer.insertText("!!!");
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 6);
        REQUIRE(link.endOffset == 11);  // Unchanged
    }
}

TEST_CASE("Hyperlink offset adjustment on delete", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(6, 11, "https://world.com");  // "World"
    
    SECTION("delete before hyperlink shifts it") {
        buffer.setCaret({0, 3});
        buffer.del();  // Delete 'l'
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 5);   // 6 - 1
        REQUIRE(link.endOffset == 10);    // 11 - 1
    }
    
    SECTION("delete within hyperlink shrinks it") {
        buffer.setCaret({0, 7});  // Inside "World"
        buffer.del();  // Delete 'o'
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 6);
        REQUIRE(link.endOffset == 10);  // 11 - 1
    }
    
    SECTION("backspace at hyperlink start shrinks it from start") {
        buffer.setCaret({0, 7});  // After 'W' in "World"
        buffer.backspace();  // Delete 'W'
        
        const auto& link = buffer.hyperlinks()[0];
        REQUIRE(link.startOffset == 6);
        REQUIRE(link.endOffset == 10);  // Shrunk
    }
    
    SECTION("deleting entire hyperlink removes it") {
        // Select "World"
        buffer.setCaret({0, 6});
        buffer.setSelectionAnchor({0, 6});
        buffer.setCaret({0, 11});
        buffer.updateSelectionToCaret();
        
        buffer.deleteSelection();
        
        REQUIRE(buffer.hyperlinks().empty());
    }
}

TEST_CASE("Hyperlink overlap handling", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(0, 5, "https://hello.com");
    
    SECTION("overlapping hyperlink replaces existing") {
        buffer.addHyperlinkAt(0, 11, "https://full.com");
        
        REQUIRE(buffer.hyperlinks().size() == 1);
        REQUIRE(buffer.hyperlinks()[0].url == "https://full.com");
        REQUIRE(buffer.hyperlinks()[0].startOffset == 0);
        REQUIRE(buffer.hyperlinks()[0].endOffset == 11);
    }
    
    SECTION("partially overlapping hyperlink replaces existing") {
        buffer.addHyperlinkAt(3, 8, "https://mid.com");
        
        REQUIRE(buffer.hyperlinks().size() == 1);
        REQUIRE(buffer.hyperlinks()[0].url == "https://mid.com");
    }
}

TEST_CASE("Hyperlink struct methods", "[hyperlink]") {
    Hyperlink link;
    link.startOffset = 5;
    link.endOffset = 10;
    link.url = "https://example.com";
    
    SECTION("contains") {
        REQUIRE(link.contains(5));
        REQUIRE(link.contains(7));
        REQUIRE(link.contains(9));
        REQUIRE_FALSE(link.contains(4));
        REQUIRE_FALSE(link.contains(10));
    }
    
    SECTION("overlaps") {
        REQUIRE(link.overlaps(0, 6));   // Overlaps start
        REQUIRE(link.overlaps(8, 15));  // Overlaps end
        REQUIRE(link.overlaps(6, 8));   // Fully inside
        REQUIRE(link.overlaps(0, 15));  // Fully contains
        REQUIRE_FALSE(link.overlaps(0, 5));   // Just before
        REQUIRE_FALSE(link.overlaps(10, 15)); // Just after
    }
    
    SECTION("length") {
        REQUIRE(link.length() == 5);
    }
    
    SECTION("equality") {
        Hyperlink link2;
        link2.startOffset = 5;
        link2.endOffset = 10;
        link2.url = "https://example.com";
        
        REQUIRE(link == link2);
        
        link2.url = "https://other.com";
        REQUIRE_FALSE(link == link2);
    }
}

TEST_CASE("Hyperlink cleared on setText", "[hyperlink]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    buffer.addHyperlinkAt(0, 5, "https://hello.com");
    REQUIRE(buffer.hyperlinks().size() == 1);
    
    buffer.setText("New text");
    REQUIRE(buffer.hyperlinks().empty());
}
